#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <string>
#include <vector>

#include <serde/serde.hpp>
#include <types/concepts.hpp>
#include <types/integers.hpp>


TEST_CASE("Serde", "[simple structures]")
{
    GIVEN("A simple struct with two chars")
    {
        struct two_chars
        {
            char a;
            char b;
        };
        std::string buffer { "cd" };
        WHEN("Deserializing it")
        {
            auto s = cpp_utils::serde::deserialize<two_chars>(buffer);
            THEN("It should have the right values")
            {
                REQUIRE(s.a == 'c');
                REQUIRE(s.b == 'd');
            }
        }
    }
    GIVEN("A simple struct with a char and a float")
    {
        struct char_float
        {
            char a;
            float b;
        };
        std::array<uint8_t, 5> buffer { 'c', 0, 0, 0xC0, 0x40 };
        WHEN("Deserializing it")
        {
            const auto s = cpp_utils::serde::deserialize<char_float>(buffer);
            THEN("It should have the right values")
            {
                REQUIRE(s.a == 'c');
                REQUIRE(s.b == 6.0f);
            }
        }
    }
}


TEST_CASE("Serde", "[enum members]")
{
    // The enum used to be written through a reference to its underlying type, which breaks
    // aliasing rules: GCC 16 with -O2 -fsanitize=thread read back the value-initialised 0.
    // Records are walked like CDFpp's record walker does, where it showed up.
    enum class kind : int32_t
    {
        none = 0,
        first = 1,
        second = 2
    };
    struct header
    {
        int64_t size;
        kind type;
    };
    std::array<uint8_t, 24> buffer { 12, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0 };
    std::vector<kind> seen;
    for (std::size_t offset = 0; offset < std::size(buffer) && std::size(seen) < 4;)
    {
        header h {};
        cpp_utils::serde::deserialize(h, buffer, offset, cpp_utils::serde::no_context {});
        switch (h.type)
        {
            case kind::first:
            case kind::second:
                seen.push_back(h.type);
                break;
            default:
                seen.push_back(kind::none);
        }
        offset += std::max(std::size_t { 1 }, static_cast<std::size_t>(h.size));
    }
    REQUIRE(seen == std::vector { kind::second, kind::first });
}

TEST_CASE("Serde", "[in place]")
{
    // Without a context, the variadic in-place deserialize used to take the struct's first field
    // as the context and read the other fields from the offset on.
    struct two_ints
    {
        int32_t a;
        int32_t b;
    };
    std::array<uint8_t, 12> buffer { 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0 };
    two_ints s {};
    REQUIRE(cpp_utils::serde::deserialize(s, buffer, 4) == 12);
    REQUIRE(s.a == 1);
    REQUIRE(s.b == 2);
    REQUIRE(cpp_utils::serde::deserialize(s, buffer) == 8);
    REQUIRE(s.a == 0);
    REQUIRE(s.b == 1);
}

TEST_CASE("Serde", "[nested structs]")
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
    std::string buffer { "abc" };
    auto s = cpp_utils::serde::deserialize<struct2>(buffer);
    REQUIRE(s.s.a == 'a');
    REQUIRE(s.s.b == 'b');
    REQUIRE(s.c == 'c');
}

TEST_CASE("Serde", "[big endian]")
{
    struct big_endian
    {
        using endianness = cpp_utils::endianness::big_endian_t;
        uint32_t a;
        uint16_t b;
    };
    static_assert(cpp_utils::reflexion::count_members<big_endian> == 2);
    static_assert(cpp_utils::serde::is_big_endian_v<big_endian>);
    std::array<uint8_t, 6> buffer { 0, 0, 0, 1, 0, 2 };
    auto s = cpp_utils::serde::deserialize<big_endian>(buffer);
    REQUIRE(s.a == 1);
    REQUIRE(s.b == 2);
}

TEST_CASE("Serde", "[array members]")
{
    struct array_members
    {
        cpp_utils::serde::static_array<uint16_t, 2> a;
        uint16_t b;
    };
    std::array<uint8_t, 6> buffer { 1, 0, 2, 0, 3, 0 };
    auto s = cpp_utils::serde::deserialize<array_members>(buffer);
    REQUIRE(s.a[0] == 1);
    REQUIRE(s.a[1] == 2);
    REQUIRE(s.b == 3);
}
