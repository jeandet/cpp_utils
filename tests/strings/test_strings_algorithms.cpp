#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <strings/algorithms.hpp>
#include <string>

TEST_CASE("Strings algorithms", "[strings]")
{
    using namespace cpp_utils::strings;
    REQUIRE(std::string("test") == make_unique_name(std::string("test"),std::vector<std::string>{"test0","test1"}));
    REQUIRE(std::string("test1") == make_unique_name(std::string("test"),std::vector<std::string>{"test","test2"}));
}
