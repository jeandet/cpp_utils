#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("unused<T> load discards the value but consumes the right bytes", "[serde]")
{
    struct with_reserved
    {
        char a;
        cpp_utils::serde::unused<int32_t> reserved;
        char b;
    };
    std::vector<uint8_t> buffer { 'x', 0xDE, 0xAD, 0xBE, 0xEF, 'y' };
    auto s = cpp_utils::serde::deserialize<with_reserved>(buffer);
    REQUIRE(s.a == 'x');
    REQUIRE(s.b == 'y');
}

TEST_CASE("unused<T> save writes zero-filled bytes of T's size", "[serde]")
{
    struct with_reserved
    {
        char a;
        cpp_utils::serde::unused<int32_t> reserved;
        char b;
    };
    with_reserved value { 'x', {}, 'y' };
    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(end_offset == 6);
    REQUIRE(sink == std::vector<uint8_t> { 'x', 0, 0, 0, 0, 'y' });
}

TEST_CASE("unused<T> wrapping a bounded_string round-trips", "[serde]")
{
    struct with_reserved_name
    {
        cpp_utils::serde::unused<cpp_utils::serde::bounded_string<4>> reserved;
        char tail;
    };
    std::vector<uint8_t> buffer { 'h', 'i', 0, 0, 'z' };
    auto s = cpp_utils::serde::deserialize<with_reserved_name>(buffer);
    REQUIRE(s.tail == 'z');

    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 0, 0, 0, 0, 'z' });
}

TEST_CASE("unused<T> load discards a genuinely dynamic-size inner field too", "[serde]")
{
    struct with_reserved_table
    {
        uint16_t count_bytes;
        cpp_utils::serde::unused<cpp_utils::serde::dynamic_array_bytes<0, uint16_t>> reserved;
        char tail;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    std::vector<uint8_t> buffer { 4, 0, 1, 0, 2, 0, 'z' };
    auto s = cpp_utils::serde::deserialize<with_reserved_table>(buffer);
    REQUIRE(s.tail == 'z');
    // save_field intentionally does not support unused<T> for dynamic-size T (see
    // static_assert in save_field) — only the load direction is exercised here.
}

TEST_CASE("unused<T> preserves and writes a caller-assigned sentinel value", "[serde]")
{
    struct reserved_field
    {
        cpp_utils::serde::unused<int32_t> rfu;
    };

    std::vector<char> data;
    cpp_utils::serde::serialize(reserved_field { cpp_utils::serde::unused<int32_t> { -1 } }, data);

    REQUIRE(data == std::vector<char> { -1, -1, -1, -1 });
}

TEST_CASE("unused<T> defaults to zero when not explicitly set", "[serde]")
{
    struct reserved_field
    {
        cpp_utils::serde::unused<int32_t> rfu;
    };

    std::vector<char> data;
    cpp_utils::serde::serialize(reserved_field {}, data);

    REQUIRE(data == std::vector<char> { 0, 0, 0, 0 });
}

TEST_CASE("unused<T> load discards bytes without touching value", "[serde]")
{
    struct reserved_field
    {
        cpp_utils::serde::unused<int32_t> rfu;
    };

    reserved_field field { cpp_utils::serde::unused<int32_t> { -1 } };
    std::vector<char> bytes(4, '\0');
    cpp_utils::serde::deserialize(field, bytes, 0);

    REQUIRE(field.rfu.value == -1);
}

TEST_CASE("unused<T>'s runtime_size matches sizeof(T)", "[serde]")
{
    struct reserved_field
    {
        cpp_utils::serde::unused<int32_t> rfu;
    };
    REQUIRE(cpp_utils::serde::runtime_size(reserved_field { cpp_utils::serde::unused<int32_t> { -1 } }) == 4);
}
