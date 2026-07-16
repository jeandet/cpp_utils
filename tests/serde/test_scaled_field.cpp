#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("scaled field load converts raw wire integer to a scaled real value", "[serde]")
{
    struct with_temperature
    {
        cpp_utils::serde::scaled<int16_t, double, 1, 100> temperature;
    };
    // raw int16_t 1234 little-endian, scale 1/100 -> 12.34
    std::vector<uint8_t> buffer { 0xD2, 0x04 };
    auto s = cpp_utils::serde::deserialize<with_temperature>(buffer);
    REQUIRE(s.temperature.value == Catch::Approx(12.34));
}

TEST_CASE("scaled field round-trip via serialize", "[serde]")
{
    struct with_temperature
    {
        cpp_utils::serde::scaled<int16_t, double, 1, 100> temperature;
    };
    with_temperature original;
    original.temperature.value = 12.34;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(original, sink, std::size_t { 0 });
    REQUIRE(end_offset == 2);
    REQUIRE(sink == std::vector<uint8_t> { 0xD2, 0x04 });

    auto roundtrip = cpp_utils::serde::deserialize<with_temperature>(sink);
    REQUIRE(roundtrip.temperature.value == Catch::Approx(12.34));
}

TEST_CASE("scaled field respects big-endian wire order", "[serde]")
{
    struct big_endian_temperature
    {
        using endianness = cpp_utils::endianness::big_endian_t;
        cpp_utils::serde::scaled<int16_t, double, 1, 100> temperature;
    };
    std::vector<uint8_t> buffer { 0x04, 0xD2 };
    auto s = cpp_utils::serde::deserialize<big_endian_temperature>(buffer);
    REQUIRE(s.temperature.value == Catch::Approx(12.34));
}

static_assert(cpp_utils::reflexion::field_size<cpp_utils::serde::scaled<int16_t, double, 1, 100>>() == 2);
static_assert(cpp_utils::serde::const_size_field<cpp_utils::serde::scaled<int16_t, double, 1, 100>>);
