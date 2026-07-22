#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <serde/serde.hpp>

using namespace cpp_utils::serde;

TEST_CASE("dynamic_array::reserve pre-allocates without changing size", "[serde]")
{
    dynamic_array<0, char> arr;
    arr.reserve(128);
    REQUIRE(arr.size() == 0);

    arr.resize(4);
    REQUIRE(arr.size() == 4);
}

TEST_CASE("dynamic_array_bytes inherits reserve", "[serde]")
{
    dynamic_array_bytes<0, char> arr;
    arr.reserve(64);
    REQUIRE(arr.size() == 0);
}
