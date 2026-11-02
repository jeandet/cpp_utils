#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <io/local_range_reader.hpp>
#include <io/range_reader.hpp>
#include <stdexcept>
#include <string>

using namespace cpp_utils::io;

static_assert(range_reader<local_range_reader>);

namespace
{
std::string write_temp_file(const std::string& name, const std::string& content)
{
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    std::ofstream { path, std::ios::binary } << content;
    return path;
}
}

TEST_CASE("local_range_reader reads several ranges in one batch and logs it", "[io]")
{
    const auto path = write_temp_file("cpp_utils_local_range_reader.bin", "0123456789abcdef");
    local_range_reader reader { path };
    REQUIRE(reader.size() == 16);

    const byte_range ranges[] { { 0, 4 }, { 8, 4 } };
    char first[4] {}, second[4] {};
    char* const destinations[] { first, second };
    reader.read_ranges(ranges, destinations);

    REQUIRE(std::string(first, 4) == "0123");
    REQUIRE(std::string(second, 4) == "89ab");
    REQUIRE(reader.batches().size() == 1);
    REQUIRE(reader.batches()[0].size() == 2);
    REQUIRE(reader.bytes_served() == 8);
    REQUIRE(reader.caller_threads().size() == 1);
    std::filesystem::remove(path);
}

TEST_CASE("copies of a local_range_reader share one call log", "[io]")
{
    const auto path = write_temp_file("cpp_utils_local_range_reader_copy.bin", "0123456789");
    local_range_reader reader { path };
    local_range_reader copy = reader;

    const byte_range ranges[] { { 2, 3 } };
    char dest[3] {};
    char* const destinations[] { dest };
    copy.read_ranges(ranges, destinations);

    REQUIRE(reader.batches().size() == 1);
    REQUIRE(reader.bytes_served() == 3);
    std::filesystem::remove(path);
}

TEST_CASE("local_range_reader throws on a range past the end", "[io]")
{
    const auto path = write_temp_file("cpp_utils_local_range_reader_end.bin", "0123456789");
    local_range_reader reader { path };

    const byte_range ranges[] { { 8, 4 } };
    char dest[4] {};
    char* const destinations[] { dest };
    REQUIRE_THROWS_AS(reader.read_ranges(ranges, destinations), std::out_of_range);
    std::filesystem::remove(path);
}

TEST_CASE("local_range_reader rejects fewer destinations than ranges", "[io]")
{
    const auto path = write_temp_file("cpp_utils_local_range_reader_dest.bin", "0123456789");
    local_range_reader reader { path };
    const byte_range ranges[] { { 0, 2 }, { 4, 2 } };
    char dest[2] {};
    char* const destinations[] { dest };
    REQUIRE_THROWS_AS(reader.read_ranges(ranges, destinations), std::invalid_argument);
    REQUIRE(reader.batches().empty());
    std::filesystem::remove(path);
}
