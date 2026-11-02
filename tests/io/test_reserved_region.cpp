#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <io/reserved_region.hpp>
#include <string>
#ifndef _WIN32
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace cpp_utils::io;

namespace
{
constexpr region_backing all_backings[] { region_backing::memory, region_backing::file };

void write_page(reserved_region& region, std::size_t page, const std::string& text)
{
    std::memcpy(region.make_writable(page, 1), text.data(), std::size(text));
    region.seal(page, 1);
}

#ifndef _WIN32
// Reads one byte at offset in a child process; true when the read killed the child.
bool read_faults(const reserved_region& region, std::size_t offset)
{
    const pid_t pid = fork();
    if (pid == 0)
    {
        std::signal(SIGSEGV, SIG_DFL);
        std::signal(SIGBUS, SIG_DFL);
        volatile char c = region.data()[offset];
        (void)c;
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFSIGNALED(status)
        && (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);
}
#endif
}

TEST_CASE("a sealed page reads back what was written", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 4 * page_size(), backing };
        write_page(region, 1, "hello");
        REQUIRE(std::string(region.data() + page_size(), 5) == "hello");
    }
}

TEST_CASE("the last partial page of a region can be sealed", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 3 * page_size() + 7, backing };
        write_page(region, 3, "tail");
        REQUIRE(region.size() == 3 * page_size() + 7);
        REQUIRE(std::string(region.data() + 3 * page_size(), 4) == "tail");
    }
}

TEST_CASE("an empty region maps nothing", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 0, backing };
        REQUIRE(region.data() == nullptr);
        REQUIRE(region.size() == 0);
    }
}

#ifndef _WIN32
TEST_CASE("pages that were never sealed fault on read", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 4 * page_size(), backing };
        write_page(region, 1, "x");
        REQUIRE_FALSE(read_faults(region, page_size()));
        REQUIRE(read_faults(region, 0));
        REQUIRE(read_faults(region, 2 * page_size()));
    }
}

TEST_CASE("make_no_access closes a sealed page again", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 2 * page_size(), backing };
        write_page(region, 0, "x");
        region.make_no_access(0, 1);
        REQUIRE(read_faults(region, 0));
    }
}

TEST_CASE("a file region in a directory that cannot be written throws", "[io]")
{
    REQUIRE_THROWS(reserved_region { page_size(), region_backing::file, "/proc" });
}
#endif
