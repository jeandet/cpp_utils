#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <io/memory_mapped_file.hpp>
#include <serde/serde.hpp>
#include <string>

#if defined(USE_MMAP) && defined(__linux__)
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace cpp_utils::io;

TEST_CASE("memory_mapped_file maps an existing file for reading", "[io]")
{
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_mmf_test.txt";
    {
        std::ofstream out(path, std::ios::binary);
        out << "hello world";
    }

    memory_mapped_file f { path.string() };

    REQUIRE(f.is_valid());
    REQUIRE(f.size() == 11);
    REQUIRE(std::string(f.data(), f.size()) == "hello world");

    std::filesystem::remove(path);
}

TEST_CASE("memory_mapped_file reports invalid for a nonexistent path", "[io]")
{
    memory_mapped_file f { "/nonexistent/path/that/should/not/exist.bin" };
    REQUIRE_FALSE(f.is_valid());
    REQUIRE(f.size() == 0);
}

TEST_CASE("memory_mapped_file::as_span exposes its data as a std::span", "[io]")
{
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_mmf_span_test.txt";
    {
        std::ofstream out(path, std::ios::binary);
        out << "hello world";
    }

    memory_mapped_file f { path.string() };
    REQUIRE(f.is_valid());

    auto bytes = f.as_span<std::byte>();
    REQUIRE(bytes.size() == 11);
    REQUIRE(std::to_integer<char>(bytes[0]) == 'h');

    auto chars = f.as_span<char>();
    REQUIRE(std::string(chars.data(), chars.size()) == "hello world");

    std::filesystem::remove(path);
}

TEST_CASE("memory_mapped_file feeds serde::deserialize directly, with no as_span() needed", "[io]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_mmf_bare_serde_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        char raw[2] = { 5, 9 };
        out.write(raw, 2);
    }

    memory_mapped_file f { path.string() };
    auto value = cpp_utils::serde::deserialize<wire_pair>(f);
    REQUIRE(value.a == 5);
    REQUIRE(value.b == 9);

    std::filesystem::remove(path);
}

TEST_CASE("memory_mapped_file::as_span feeds serde::deserialize directly", "[io]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_mmf_serde_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        char raw[2] = { 5, 9 };
        out.write(raw, 2);
    }

    memory_mapped_file f { path.string() };
    auto value = cpp_utils::serde::deserialize<wire_pair>(f.as_span<std::byte>());
    REQUIRE(value.a == 5);
    REQUIRE(value.b == 9);

    std::filesystem::remove(path);
}

#ifdef USE_MMAP
TEST_CASE("a failed mmap() is not mistaken for a valid mapping", "[io]")
{
    REQUIRE(cpp_utils::io::details::checked_mmap(MAP_FAILED) == nullptr);

    int dummy = 0;
    REQUIRE(cpp_utils::io::details::checked_mmap(&dummy) == reinterpret_cast<char*>(&dummy));
}
#endif

// Linux-only: forces a real open()-succeeds/mmap()-fails run through the
// constructor by capping the child's address space (RLIMIT_AS) below what
// mapping the file needs, to pin down the close(fd); fd = -1; cleanup path.
// Isolated to a forked child so the cap can't affect the rest of the test
// binary. Not run on macOS/BSD: RLIMIT_AS enforcement for mmap() there is
// not verified to be as deterministic as on Linux.
#if defined(USE_MMAP) && defined(__linux__)
TEST_CASE("memory_mapped_file cleans up its fd when mmap() itself fails", "[io]")
{
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_mmf_rlimit_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        std::string payload(64 * 1024 * 1024, 'x');
        out << payload;
    }

    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0)
    {
        rlimit limit { 32UL * 1024 * 1024, 32UL * 1024 * 1024 };
        if (setrlimit(RLIMIT_AS, &limit) != 0)
            _exit(10);

        memory_mapped_file f { path.string() };
        if (f.is_valid())
            _exit(1);
        if (f.mapped_file != nullptr)
            _exit(2);
        if (f.fd != -1)
            _exit(3);
        _exit(0);
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);

    std::filesystem::remove(path);
}
#endif
