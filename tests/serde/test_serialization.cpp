#define CATCH_CONFIG_MAIN
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

TEST_CASE("Serialize", "[simple structures]")
{
    GIVEN("A simple struct with two chars")
    {
        struct two_chars
        {
            char a;
            char b;
        };
        two_chars value { 'c', 'd' };
        WHEN("Serializing it")
        {
            std::vector<uint8_t> sink;
            auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
            THEN("It should produce the right bytes and round-trip through deserialize")
            {
                REQUIRE(end_offset == 2);
                REQUIRE(sink.size() == 2);
                REQUIRE(sink[0] == 'c');
                REQUIRE(sink[1] == 'd');
                auto roundtrip = cpp_utils::serde::deserialize<two_chars>(sink);
                REQUIRE(roundtrip.a == 'c');
                REQUIRE(roundtrip.b == 'd');
            }
        }
    }
}

TEST_CASE("Serialize", "[nested structs]")
{
    struct struct2
    {
        struct struct1
        {
            char a;
            char b;
        } s;
        char c;
    };
    struct2 value { { 'a', 'b' }, 'c' };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink.size() == 3);
    auto roundtrip = cpp_utils::serde::deserialize<struct2>(sink);
    REQUIRE(roundtrip.s.a == 'a');
    REQUIRE(roundtrip.s.b == 'b');
    REQUIRE(roundtrip.c == 'c');
}

TEST_CASE("Serialize", "[big endian fundamental]")
{
    struct big_endian
    {
        using endianness = cpp_utils::endianness::big_endian_t;
        uint32_t a;
        uint16_t b;
    };
    big_endian value { 1, 2 };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 0, 0, 0, 1, 0, 2 });
    auto roundtrip = cpp_utils::serde::deserialize<big_endian>(sink);
    REQUIRE(roundtrip.a == 1);
    REQUIRE(roundtrip.b == 2);
}

TEST_CASE("Serialize", "[static array]")
{
    struct array_members
    {
        cpp_utils::serde::static_array<uint16_t, 2> a;
        uint16_t b;
    };
    array_members value {};
    value.a[0] = 1;
    value.a[1] = 2;
    value.b = 3;
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 1, 0, 2, 0, 3, 0 });
    auto roundtrip = cpp_utils::serde::deserialize<array_members>(sink);
    REQUIRE(roundtrip.a[0] == 1);
    REQUIRE(roundtrip.a[1] == 2);
    REQUIRE(roundtrip.b == 3);
}

TEST_CASE("Serialize", "[padding]")
{
    struct with_padding
    {
        char a;
        cpp_utils::serde::padding_bytes_t<3, 0xFF> pad;
        char b;
    };
    with_padding value { 'x', {}, 'y' };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 'x', 0xFF, 0xFF, 0xFF, 'y' });
    auto roundtrip = cpp_utils::serde::deserialize<with_padding>(sink);
    REQUIRE(roundtrip.a == 'x');
    REQUIRE(roundtrip.b == 'y');
}
