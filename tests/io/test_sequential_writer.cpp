#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <io/sequential_writer.hpp>
#include <string>
#include <vector>

using namespace cpp_utils::io;

TEST_CASE("vector_writer appends bytes and grows its container", "[io]")
{
    std::vector<char> data;
    vector_writer writer { data };

    REQUIRE(writer.write("ab", 2) == 2);
    REQUIRE(writer.write("cd", 2) == 4);
    REQUIRE(writer.offset() == 4);
    REQUIRE(std::string(data.data(), data.size()) == "abcd");
}

TEST_CASE("vector_writer::fill appends N copies of a byte", "[io]")
{
    std::vector<char> data;
    vector_writer writer { data };

    REQUIRE(writer.fill('\0', 3) == 3);
    REQUIRE(data == std::vector<char> { 0, 0, 0 });
}

TEST_CASE("vector_writer works over any resize/data/size container", "[io]")
{
    struct minimal_container
    {
        std::vector<char> backing;
        void resize(std::size_t n) { backing.resize(n); }
        char* data() { return backing.data(); }
        std::size_t size() const { return backing.size(); }
    };
    minimal_container data;
    vector_writer writer { data };

    writer.write("xy", 2);
    REQUIRE(std::string(data.backing.data(), data.backing.size()) == "xy");
}

TEST_CASE("file_writer streams bytes directly to disk", "[io]")
{
    auto path = std::filesystem::temp_directory_path() / "cpp_utils_file_writer_test.bin";
    {
        file_writer writer { path.string() };
        writer.write("hello", 5);
        writer.fill('!', 2);
    }

    std::ifstream in(path, std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE(contents == "hello!!");

    std::filesystem::remove(path);
}

TEST_CASE("vector_writer and file_writer both satisfy sequential_writer", "[io]")
{
    static_assert(sequential_writer<vector_writer<std::vector<char>>>);
    static_assert(sequential_writer<file_writer>);
    REQUIRE(true);
}
