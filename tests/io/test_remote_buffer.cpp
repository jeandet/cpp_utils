#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <io/local_range_reader.hpp>
#include <io/remote_buffer.hpp>
#include <latch>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifndef _WIN32
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace cpp_utils::io;

namespace
{
constexpr std::size_t KiB = 1024;
constexpr std::size_t window = 256 * KiB;
constexpr std::size_t big_size = 5 * window + 123;

char pattern(std::size_t i) { return static_cast<char>((i * 7 + i / 251) & 0xFF); }

std::string write_pattern_file(const std::string& name, std::size_t size)
{
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    std::string content(size, '\0');
    for (std::size_t i = 0; i < size; ++i)
        content[i] = pattern(i);
    std::ofstream { path, std::ios::binary } << content;
    return path;
}

void set_env(const char* name, const char* value)
{
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

bool bytes_match(const char* data, std::size_t offset, std::size_t size)
{
    for (std::size_t i = offset; i < offset + size; ++i)
        if (data[i] != pattern(i))
            return false;
    return true;
}

// Throws on the n-th call until healed; copies share the switch.
struct flaky_reader
{
    local_range_reader inner;
    std::shared_ptr<std::atomic<int>> calls = std::make_shared<std::atomic<int>>(0);
    std::shared_ptr<std::atomic<bool>> healed = std::make_shared<std::atomic<bool>>(false);
    int failing_call = 2;

    std::size_t size() const { return inner.size(); }
    void read_ranges(std::span<const byte_range> ranges, std::span<char* const> dest) const
    {
        if (++*calls == failing_call && !*healed)
            throw std::runtime_error { "transport failure" };
        inner.read_ranges(ranges, dest);
    }
};
static_assert(range_reader<flaky_reader>);

#ifndef _WIN32
bool read_faults(const char* p)
{
    const pid_t pid = fork();
    if (pid == 0)
    {
        std::signal(SIGSEGV, SIG_DFL);
        std::signal(SIGBUS, SIG_DFL);
        volatile char c = *p;
        (void)c;
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFSIGNALED(status) && (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);
}
#endif

struct temp_file
{
    std::string path;
    temp_file(const std::string& name, std::size_t size) : path { write_pattern_file(name, size) }
    {
    }
    ~temp_file() { std::filesystem::remove(path); }
};
}

TEST_CASE("the first ensure fetches the read-ahead window and the tail in one batch", "[io]")
{
    temp_file f { "cpp_utils_rb_first.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };

    buffer.ensure(10, 4);

    REQUIRE(bytes_match(buffer.data(), 10, 4));
    const auto batches = reader.batches();
    REQUIRE(batches.size() == 1);
    REQUIRE(batches[0].size() == 2);
    REQUIRE(batches[0][0].offset == 0);
    REQUIRE(batches[0][0].size == window);
    REQUIRE(batches[0][1].offset == (big_size - window) / page_size() * page_size());
    REQUIRE(batches[0][1].offset + batches[0][1].size == big_size);
}

TEST_CASE("bytes already fetched are not fetched again", "[io]")
{
    temp_file f { "cpp_utils_rb_again.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };

    buffer.ensure(10, 4);
    buffer.ensure(20, 4);

    REQUIRE(reader.batches().size() == 1);
}

TEST_CASE("one ensure of far-apart ranges sends one batch with two runs", "[io]")
{
    temp_file f { "cpp_utils_rb_two_runs.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader, remote_buffer_options { .tail_prefetch = 0 } };

    const byte_range ranges[] { { 10, 4 }, { 3 * window + 5, 4 } };
    buffer.ensure(ranges);

    REQUIRE(reader.batches().size() == 1);
    REQUIRE(reader.batches()[0].size() == 2);
    REQUIRE(bytes_match(buffer.data(), 3 * window + 5, 4));
}

TEST_CASE("check_present throws until the bytes are fetched", "[io]")
{
    temp_file f { "cpp_utils_rb_present.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader, remote_buffer_options { .tail_prefetch = 0 } };

    REQUIRE_THROWS_AS(buffer.check_present(3 * window, 1), std::runtime_error);
    buffer.ensure(3 * window, 1);
    REQUIRE_NOTHROW(buffer.check_present(3 * window, 1));
}

TEST_CASE("ranges outside the file throw without overflowing", "[io]")
{
    temp_file f { "cpp_utils_rb_bounds.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };

    REQUIRE_THROWS_AS(buffer.ensure(big_size, 1), std::runtime_error);
    REQUIRE_THROWS_AS(buffer.ensure(big_size - 1, 10), std::runtime_error);
    constexpr auto huge = std::numeric_limits<std::size_t>::max();
    REQUIRE_THROWS_AS(buffer.ensure(huge - 10, 20), std::runtime_error);
    REQUIRE(reader.batches().empty());
}

TEST_CASE("a file smaller than the tail is fetched whole, without underflow", "[io]")
{
    temp_file f { "cpp_utils_rb_small.bin", 1000 };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };

    buffer.ensure(0, 1);

    REQUIRE(reader.bytes_served() == 1000);
    REQUIRE(bytes_match(buffer.data(), 0, 1000));
}

TEST_CASE("no byte is fetched twice over random ensures", "[io]")
{
    temp_file f { "cpp_utils_rb_random.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader, remote_buffer_options { .read_ahead = 4 * KiB } };

    std::mt19937 rng { 42 };
    std::uniform_int_distribution<std::size_t> offsets { 0, big_size - 1 };
    for (int i = 0; i < 200; ++i)
    {
        const auto offset = offsets(rng);
        const auto size = std::min<std::size_t>(1 + offsets(rng) % 20000, big_size - offset);
        buffer.ensure(offset, size);
        REQUIRE(bytes_match(buffer.data(), offset, size));
    }
    REQUIRE(reader.bytes_served() <= big_size);
}

TEST_CASE("a failed fetch leaves its pages unreadable and can be retried", "[io]")
{
    temp_file f { "cpp_utils_rb_flaky.bin", big_size };
    flaky_reader reader { local_range_reader { f.path } };
    remote_buffer buffer { reader, remote_buffer_options { .tail_prefetch = 0 } };

    buffer.ensure(0, 1);
    REQUIRE_THROWS_AS(buffer.ensure(3 * window, 1), std::runtime_error);
    REQUIRE_THROWS_AS(buffer.check_present(3 * window, 1), std::runtime_error);
#ifndef _WIN32
    REQUIRE(read_faults(buffer.data() + 3 * window));
#endif
    *reader.healed = true;
    buffer.ensure(3 * window, 1);
    REQUIRE(bytes_match(buffer.data(), 3 * window, 1));
}

TEST_CASE("threads ensuring the same range share one fetch", "[io]")
{
    temp_file f { "cpp_utils_rb_threads.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };

    std::vector<std::thread> threads;
    std::atomic<int> matches = 0;
    std::latch start { 8 };
    for (int i = 0; i < 8; ++i)
        threads.emplace_back(
            [&]
            {
                start.arrive_and_wait();
                buffer.ensure(2 * window, 100);
                matches += bytes_match(buffer.data(), 2 * window, 100);
            });
    for (auto& t : threads)
        t.join();

    REQUIRE(matches == 8);
    REQUIRE(reader.batches().size() == 1);
    REQUIRE(reader.caller_threads().size() == 1);
}

TEST_CASE("download_whole_below fetches the whole file only when it is smaller", "[io]")
{
    temp_file f { "cpp_utils_rb_whole.bin", big_size };
    {
        local_range_reader reader { f.path };
        remote_buffer buffer { reader,
            remote_buffer_options { .tail_prefetch = 0, .download_whole_below = big_size + 1 } };
        buffer.ensure(0, 1);
        REQUIRE(reader.bytes_served() == big_size);
    }
    {
        local_range_reader reader { f.path };
        remote_buffer buffer { reader,
            remote_buffer_options { .tail_prefetch = 0, .download_whole_below = big_size } };
        buffer.ensure(0, 1);
        REQUIRE(reader.bytes_served() < big_size);
    }
}

TEST_CASE("a remote buffer over an empty source is invalid", "[io]")
{
    temp_file f { "cpp_utils_rb_empty.bin", 0 };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };
    REQUIRE_FALSE(buffer.is_valid());
}

TEST_CASE("read copies through ensure", "[io]")
{
    temp_file f { "cpp_utils_rb_read.bin", big_size };
    local_range_reader reader { f.path };
    const remote_buffer buffer { reader };
    char dest[16];
    buffer.read(dest, 4 * window, 16);
    REQUIRE(bytes_match(buffer.data(), 4 * window, 16));
    for (std::size_t i = 0; i < 16; ++i)
        REQUIRE(dest[i] == pattern(4 * window + i));
}

TEST_CASE("options can come from environment variables", "[io]")
{
    set_env("X_READ_AHEAD", "65536");
    set_env("X_BACKING", "file");
    const auto options = remote_buffer_options::from_env("X_");
    REQUIRE(options.read_ahead == 65536);
    REQUIRE(options.backing == region_backing::file);

    set_env("X_READ_AHEAD", "abc");
    REQUIRE_THROWS_AS(remote_buffer_options::from_env("X_"), std::invalid_argument);
    set_env("X_READ_AHEAD", "");
    set_env("X_BACKING", "");
}

namespace
{
struct huge_reader
{
    std::size_t claimed_size;
    std::size_t size() const { return claimed_size; }
    void read_ranges(std::span<const byte_range>, std::span<char* const>) const
    {
        throw std::logic_error { "must not be called" };
    }
};
}

TEST_CASE("a reader claiming an impossible size is rejected", "[io]")
{
    constexpr auto huge = std::numeric_limits<std::size_t>::max();
    REQUIRE_THROWS_AS(remote_buffer(huge_reader { huge }), std::length_error);
    REQUIRE_THROWS_AS(remote_buffer(huge_reader { huge - 100 }), std::length_error);
}

TEST_CASE("the tail is still fetched when the first fetch failed", "[io]")
{
    temp_file f { "cpp_utils_rb_first_fails.bin", big_size };
    flaky_reader reader { local_range_reader { f.path }, std::make_shared<std::atomic<int>>(0),
        std::make_shared<std::atomic<bool>>(false), 1 };
    remote_buffer buffer { reader };

    REQUIRE_THROWS_AS(buffer.ensure(0, 1), std::runtime_error);
    *reader.healed = true;
    buffer.ensure(0, 1);
    const auto batches = reader.inner.batches();
    REQUIRE(batches.size() == 1);
    REQUIRE(batches[0].size() == 2);
    REQUIRE(batches[0][1].offset + batches[0][1].size == big_size);
}

TEST_CASE("an empty request fetches nothing", "[io]")
{
    temp_file f { "cpp_utils_rb_empty_request.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader };
    char dest[1];
    buffer.ensure(10, 0);
    buffer.read(dest, 10, 0);
    REQUIRE(reader.batches().empty());
}

TEST_CASE("read_ahead is rounded up to whole pages", "[io]")
{
    temp_file f { "cpp_utils_rb_rounding.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader,
        remote_buffer_options { .read_ahead = page_size() + 1, .tail_prefetch = 0 } };
    buffer.ensure(0, 1);
    REQUIRE(reader.bytes_served() == 2 * page_size());
}

TEST_CASE("an absurd read_ahead is rejected", "[io]")
{
    temp_file f { "cpp_utils_rb_absurd.bin", big_size };
    local_range_reader reader { f.path };
    REQUIRE_THROWS_AS(remote_buffer(reader,
                          remote_buffer_options { .read_ahead
                              = std::numeric_limits<std::size_t>::max() }),
        std::invalid_argument);
}

TEST_CASE("a file-backed remote buffer reads back its bytes", "[io]")
{
    temp_file f { "cpp_utils_rb_file_backing.bin", big_size };
    local_range_reader reader { f.path };
    remote_buffer buffer { reader,
        remote_buffer_options {
            .backing = region_backing::file, .temp_dir = std::filesystem::temp_directory_path() } };
    buffer.ensure(3 * window, 100);
    REQUIRE(bytes_match(buffer.data(), 3 * window, 100));
}

namespace
{
// A reader that holds its fetch until released, and counts the waits it is asked to run.
struct waiting_reader
{
    local_range_reader inner;
    std::shared_ptr<std::atomic<int>> waits = std::make_shared<std::atomic<int>>(0);
    std::shared_ptr<std::latch> fetching = std::make_shared<std::latch>(1);
    std::shared_ptr<std::latch> release = std::make_shared<std::latch>(1);

    std::size_t size() const { return inner.size(); }
    void read_ranges(std::span<const byte_range> ranges, std::span<char* const> dest) const
    {
        fetching->count_down();
        release->wait();
        inner.read_ranges(ranges, dest);
    }
    template <typename F>
    void blocking_wait(F&& wait) const
    {
        ++*waits;
        wait();
    }
};
}

TEST_CASE("waiting for another thread's fetch goes through the reader's blocking_wait", "[io]")
{
    temp_file f { "cpp_utils_rb_blocking_wait.bin", big_size };
    waiting_reader reader { local_range_reader { f.path } };
    remote_buffer buffer { reader };

    std::thread first { [&] { buffer.ensure(2 * window, 100); } };
    reader.fetching->wait();
    std::thread second { [&] { buffer.ensure(2 * window, 100); } };
    while (*reader.waits == 0)
        std::this_thread::yield();
    reader.release->count_down();
    first.join();
    second.join();

    REQUIRE(*reader.waits >= 1);
    REQUIRE(reader.inner.batches().size() == 1);
}
