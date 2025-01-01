#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <memory>
#include <string>
#include <vector>
#include <sstream>

namespace cpp_utils::reflexion
{

void load_value(const char* input, std::size_t offset, void* dest, std::size_t count)
{
    memcpy(dest, input+offset, count);
}

}

#include <reflexion/reflection.hpp>

TEST_CASE("Print tree", "[trees]")
{
    struct two_chars
    {
        char a;
        char b;
    };
    THEN("we can load it from a buffer")
    {
        std::string buffer { "cd" };
        auto s= cpp_utils::reflexion::load_record<two_chars>( buffer.c_str(), 0);
        REQUIRE(s.a == 'c');
        REQUIRE(s.b == 'd');

    }
}

