#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

TEST_CASE("bounded_string load", "[serde]")
{
    struct with_name
    {
        cpp_utils::serde::bounded_string<8> name;
        uint16_t value;
    };
    std::vector<uint8_t> buffer { 'a', 'b', 'c', 0, 0, 0, 0, 0, 5, 0 };
    auto s = cpp_utils::serde::deserialize<with_name>(buffer);
    REQUIRE(s.name.value == "abc");
    REQUIRE(s.value == 5);
}

TEST_CASE("bounded_string load stops at embedded null even mid-buffer", "[serde]")
{
    struct just_name
    {
        cpp_utils::serde::bounded_string<4> name;
    };
    std::vector<uint8_t> buffer { 'h', 'i', 0, 'z' };
    auto s = cpp_utils::serde::deserialize<just_name>(buffer);
    REQUIRE(s.name.value == "hi");
}

TEST_CASE("bounded_string round-trip via serialize", "[serde]")
{
    struct with_name
    {
        cpp_utils::serde::bounded_string<8> name;
        uint16_t value;
    };
    with_name original;
    original.name.value = "abc";
    original.value = 5;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(original, sink, std::size_t { 0 });
    REQUIRE(end_offset == 10);
    REQUIRE(sink == std::vector<uint8_t> { 'a', 'b', 'c', 0, 0, 0, 0, 0, 5, 0 });

    auto roundtrip = cpp_utils::serde::deserialize<with_name>(sink);
    REQUIRE(roundtrip.name.value == "abc");
    REQUIRE(roundtrip.value == 5);
}

static_assert(cpp_utils::reflexion::field_size<cpp_utils::serde::bounded_string<8>>() == 8);
static_assert(cpp_utils::serde::const_size_field<cpp_utils::serde::bounded_string<8>>);
