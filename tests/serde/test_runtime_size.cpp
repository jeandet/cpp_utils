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
