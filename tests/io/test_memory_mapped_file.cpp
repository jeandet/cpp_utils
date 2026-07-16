#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <io/memory_mapped_file.hpp>
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

#ifdef USE_MMAP
TEST_CASE("a failed mmap() is not mistaken for a valid mapping", "[io]")
{
    REQUIRE(cpp_utils::io::details::checked_mmap(MAP_FAILED) == nullptr);

    int dummy = 0;
    REQUIRE(cpp_utils::io::details::checked_mmap(&dummy) == reinterpret_cast<char*>(&dummy));
}
#endif
