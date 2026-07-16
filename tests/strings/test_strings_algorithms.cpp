#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <strings/algorithms.hpp>
#include <strings/traits.hpp>
#include <string>

TEST_CASE("Strings algorithms", "[strings]")
{
    using namespace cpp_utils::strings;
    REQUIRE(std::string("test") == make_unique_name(std::string("test"),std::vector<std::string>{"test0","test1"}));
    REQUIRE(std::string("test1") == make_unique_name(std::string("test"),std::vector<std::string>{"test","test2"}));

    REQUIRE(std::string("test1") == trim(std::string("      test1     ")));
    REQUIRE(std::string("test1") == trim(std::string("test1     ")));
    REQUIRE(std::string("test1") == trim(std::string("      test1")));
    REQUIRE(std::string("test1") == trim(std::string("test1")));
    REQUIRE(std::string("") == trim(std::string("    ")));
    REQUIRE(std::string("") == trim(std::string("")));
}

TEST_CASE("split/join are discoverable from cpp_utils::strings", "[strings]")
{
    using namespace cpp_utils::strings;
    std::vector<std::string> parts;
    split(std::string("a,b,c"), parts, ',');
    REQUIRE(parts == std::vector<std::string> { "a", "b", "c" });
    REQUIRE(std::string("a,b,c") == join(parts, ','));
}

TEST_CASE("is_string_like_v detects string-like types", "[strings]")
{
    using namespace cpp_utils::strings;
    REQUIRE(is_string_like_v<std::string>);
    REQUIRE(is_string_like_v<std::string_view>);
    REQUIRE(is_string_like_v<const char*>);
    REQUIRE_FALSE(is_string_like_v<int>);
    REQUIRE_FALSE(is_string_like_v<std::vector<int>>);
}
