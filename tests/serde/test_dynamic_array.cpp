#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("dynamic_array (element count) sized from the parent record round-trips", "[serde]")
{
    struct with_table
    {
        uint16_t element_count;
        cpp_utils::serde::dynamic_array<0, uint16_t> values;

        std::size_t field_size(const cpp_utils::serde::dynamic_array<0, uint16_t>&) const
        {
            return element_count;
        }
    };
    // element_count = 2 (LE u16), followed by 2 u16 elements: unlike
    // dynamic_array_bytes this is an element count, not a byte count.
    std::vector<uint8_t> buffer { 2, 0, 1, 0, 2, 0 };
    auto s = cpp_utils::serde::deserialize<with_table>(buffer);
    REQUIRE(s.values.size() == 2);
    REQUIRE(s.values[0] == 1);
    REQUIRE(s.values[1] == 2);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 6);
    REQUIRE(sink == buffer);
}

TEST_CASE("dynamic_array with zero elements round-trips to no element bytes", "[serde]")
{
    struct with_table
    {
        uint8_t element_count;
        cpp_utils::serde::dynamic_array<0, uint16_t> values;

        std::size_t field_size(const cpp_utils::serde::dynamic_array<0, uint16_t>&) const
        {
            return element_count;
        }
    };
    std::vector<uint8_t> buffer { 0 };
    auto s = cpp_utils::serde::deserialize<with_table>(buffer);
    REQUIRE(s.values.size() == 0);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 1);
    REQUIRE(sink == buffer);
}

TEST_CASE("dynamic_array of compound elements round-trips", "[serde]")
{
    struct point
    {
        uint8_t x;
        uint8_t y;
    };
    struct with_points
    {
        uint8_t point_count;
        cpp_utils::serde::dynamic_array<0, point> points;

        std::size_t field_size(const cpp_utils::serde::dynamic_array<0, point>&) const
        {
            return point_count;
        }
    };
    std::vector<uint8_t> buffer { 2, 1, 2, 3, 4 };
    auto s = cpp_utils::serde::deserialize<with_points>(buffer);
    REQUIRE(s.points.size() == 2);
    REQUIRE(s.points[0].x == 1);
    REQUIRE(s.points[0].y == 2);
    REQUIRE(s.points[1].x == 3);
    REQUIRE(s.points[1].y == 4);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 5);
    REQUIRE(sink == buffer);
}

TEST_CASE("dynamic_array_until_eof consumes remaining fundamental elements", "[serde]")
{
    struct trailing_u16
    {
        uint8_t header;
        cpp_utils::serde::dynamic_array_until_eof<uint16_t> values;
    };
    std::vector<uint8_t> buffer { 9, 1, 0, 2, 0, 3, 0 };
    auto s = cpp_utils::serde::deserialize<trailing_u16>(buffer);
    REQUIRE(s.header == 9);
    REQUIRE(s.values.size() == 3);
    REQUIRE(s.values[0] == 1);
    REQUIRE(s.values[1] == 2);
    REQUIRE(s.values[2] == 3);
}

TEST_CASE("dynamic_array_until_eof consumes remaining constant-size compound elements", "[serde]")
{
    struct point
    {
        uint8_t x;
        uint8_t y;
    };
    struct trailing_points
    {
        uint8_t header;
        cpp_utils::serde::dynamic_array_until_eof<point> points;
    };
    std::vector<uint8_t> buffer { 5, 1, 2, 3, 4, 5, 6 };
    auto s = cpp_utils::serde::deserialize<trailing_points>(buffer);
    REQUIRE(s.header == 5);
    REQUIRE(s.points.size() == 3);
    REQUIRE(s.points[0].x == 1);
    REQUIRE(s.points[0].y == 2);
    REQUIRE(s.points[1].x == 3);
    REQUIRE(s.points[1].y == 4);
    REQUIRE(s.points[2].x == 5);
    REQUIRE(s.points[2].y == 6);
}

TEST_CASE(
    "dynamic_array_until_eof consumes remaining self-describing variable-length records",
    "[serde]")
{
    struct record
    {
        uint8_t len;
        cpp_utils::serde::dynamic_array_bytes<0, uint8_t> payload;

        std::size_t field_size(const cpp_utils::serde::dynamic_array_bytes<0, uint8_t>&) const
        {
            return len;
        }
    };
    struct records_until_eof
    {
        cpp_utils::serde::dynamic_array_until_eof<record> records;
    };
    std::vector<uint8_t> buffer { 2, 0xAA, 0xBB, 1, 0xCC };
    auto s = cpp_utils::serde::deserialize<records_until_eof>(buffer);
    REQUIRE(s.records.size() == 2);
    REQUIRE(s.records[0].len == 2);
    REQUIRE(s.records[0].payload.size() == 2);
    REQUIRE(s.records[0].payload[0] == 0xAA);
    REQUIRE(s.records[0].payload[1] == 0xBB);
    REQUIRE(s.records[1].len == 1);
    REQUIRE(s.records[1].payload.size() == 1);
    REQUIRE(s.records[1].payload[0] == 0xCC);
}
