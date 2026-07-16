#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("runtime_size matches serialize output length for const-size composites", "[serde]")
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
    REQUIRE(cpp_utils::serde::runtime_size(value) == 3);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(cpp_utils::serde::runtime_size(value) == end_offset);
}

TEST_CASE("runtime_size matches serialize output length for dynamic fields", "[serde]")
{
    struct with_table
    {
        uint16_t count_bytes;
        cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    with_table value {};
    value.count_bytes = 4;
    value.values.resize(2);
    value.values[0] = 1;
    value.values[1] = 2;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(cpp_utils::serde::runtime_size(value) == end_offset);
    REQUIRE(cpp_utils::serde::runtime_size(value) == 6);
}

TEST_CASE(
    "runtime_size matches serialize output length for dynamic_array of non-constant-size "
    "compound elements",
    "[serde]")
{
    struct inner
    {
        uint8_t count_bytes;
        cpp_utils::serde::dynamic_array_bytes<0, uint8_t> values;
    };
    struct outer
    {
        uint8_t element_count;
        cpp_utils::serde::dynamic_array<0, inner> elements;
    };

    outer value {};
    value.element_count = 2;
    value.elements.resize(2);
    value.elements[0].count_bytes = 1;
    value.elements[0].values.resize(1);
    value.elements[0].values[0] = 42;
    value.elements[1].count_bytes = 2;
    value.elements[1].values.resize(2);
    value.elements[1].values[0] = 1;
    value.elements[1].values[1] = 2;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(cpp_utils::serde::runtime_size(value) == end_offset);
}

TEST_CASE("serialize/deserialize/runtime_size support an empty (0-member) composite", "[serde]")
{
    struct Empty
    {
    };

    Empty value {};
    REQUIRE(cpp_utils::serde::runtime_size(value) == 0);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(end_offset == 0);

    auto roundtrip = cpp_utils::serde::deserialize<Empty>(sink);
    (void)roundtrip;
}
