#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <limits>
#include <io/reserved_region.hpp>
#include <cstdint>
#include <fstream>
#include <sstream>
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
// Touches one byte at offset in a child process; true when that killed the child.
bool access_faults(const reserved_region& region, std::size_t offset, bool write)
{
    const pid_t pid = fork();
    if (pid == 0)
    {
        std::signal(SIGSEGV, SIG_DFL);
        std::signal(SIGBUS, SIG_DFL);
        if (write)
            region.data()[offset] = 'w';
        volatile char c = region.data()[offset];
        (void)c;
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFSIGNALED(status)
        && (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);
}

bool read_faults(const reserved_region& region, std::size_t offset)
{
    return access_faults(region, offset, false);
}

#ifdef __linux__
// The /proc/self/maps line of the mapping holding p.
std::string mapping_of(const char* p)
{
    std::ifstream maps { "/proc/self/maps" };
    const auto address = reinterpret_cast<std::uintptr_t>(p);
    for (std::string line; std::getline(maps, line);)
    {
        std::uintptr_t begin = 0, end = 0;
        char dash = 0;
        std::istringstream { line } >> std::hex >> begin >> dash >> end;
        if (begin <= address && address < end)
            return line;
    }
    return {};
}
#endif
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

#ifndef _WIN32
TEST_CASE("a sealed page cannot be written", "[io]")
{
    for (auto backing : all_backings)
    {
        reserved_region region { 2 * page_size(), backing };
        write_page(region, 1, "x");
        REQUIRE(access_faults(region, page_size(), true));
    }
}
#endif

#ifdef __linux__
TEST_CASE("file backing maps a deleted temp file, memory backing does not", "[io]")
{
    reserved_region file_region { page_size(), region_backing::file,
        std::filesystem::temp_directory_path() };
    reserved_region memory_region { page_size(), region_backing::memory };
    REQUIRE(mapping_of(file_region.data()).find("cpp_utils_region_") != std::string::npos);
    REQUIRE(mapping_of(file_region.data()).find("(deleted)") != std::string::npos);
    REQUIRE(mapping_of(memory_region.data()).find("cpp_utils_region_") == std::string::npos);
}
#endif

TEST_CASE("page ranges outside the region throw without overflowing", "[io]")
{
    reserved_region region { 2 * page_size() };
    REQUIRE_THROWS_AS(region.seal(2, 1), std::out_of_range);
    REQUIRE_THROWS_AS(region.seal(std::numeric_limits<std::size_t>::max(), 2), std::out_of_range);
    REQUIRE_THROWS_AS(region.seal(0, 0), std::out_of_range);
}
