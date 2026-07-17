#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <types/dtype_dispatch.hpp>

using namespace cpp_utils::types;

TEST_CASE("dispatch_dtype dispatches known format codes to the matching type", "[types]")
{
    auto size_of_code = [](char code)
    {
        return dispatch_dtype(
            code, [](auto type_tag) { return sizeof(typename decltype(type_tag)::type); });
    };

    REQUIRE(size_of_code('f') == sizeof(float));
    REQUIRE(size_of_code('d') == sizeof(double));
    REQUIRE(size_of_code('b') == sizeof(int8_t));
    REQUIRE(size_of_code('B') == sizeof(uint8_t));
    REQUIRE(size_of_code('h') == sizeof(int16_t));
    REQUIRE(size_of_code('H') == sizeof(uint16_t));
    REQUIRE(size_of_code('i') == sizeof(int32_t));
    REQUIRE(size_of_code('I') == sizeof(uint32_t));
    REQUIRE(size_of_code('l') == sizeof(long));
    REQUIRE(size_of_code('L') == sizeof(unsigned long));
    REQUIRE(size_of_code('q') == sizeof(long long));
    REQUIRE(size_of_code('Q') == sizeof(unsigned long long));
}

TEST_CASE("dispatch_dtype throws on an unknown format code", "[types]")
{
    REQUIRE_THROWS_AS(dispatch_dtype('z', [](auto) { return 0; }), std::invalid_argument);
}
