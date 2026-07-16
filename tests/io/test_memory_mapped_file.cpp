#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <io/memory_mapped_file.hpp>
#include <serde/serde.hpp>
#include <string>

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
