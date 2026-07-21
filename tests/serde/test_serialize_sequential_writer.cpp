#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <io/sequential_writer.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

using namespace cpp_utils::serde;
using cpp_utils::io::vector_writer;

TEST_CASE("serialize writes fundamental fields through a sequential_writer sink", "[serde]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };
    std::vector<char> data;
    vector_writer writer { data };

    auto offset = serialize(wire_pair { 5, 9 }, writer);

    REQUIRE(offset == 2);
    REQUIRE(data == std::vector<char> { 5, 9 });
}

TEST_CASE("serialize writes a bounded_string through a sequential_writer sink", "[serde]")
{
    struct with_name
    {
        bounded_string<4> name;
    };
    std::vector<char> data;
    vector_writer writer { data };

    serialize(with_name { { "hi" } }, writer);

    REQUIRE(data == std::vector<char> { 'h', 'i', '\0', '\0' });
}

TEST_CASE("serialize writes padding_bytes_t through a sequential_writer sink", "[serde]")
{
    struct padded
    {
        uint8_t a;
        padding_bytes_t<2, 0xAB> pad;
    };
    std::vector<char> data;
    vector_writer writer { data };

    serialize(padded { 1, {} }, writer);

    REQUIRE(data == std::vector<char> { 1, static_cast<char>(0xAB), static_cast<char>(0xAB) });
}

TEST_CASE("serialize and byte_sink-based serialize agree on output bytes", "[serde]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };
    std::vector<char> via_writer;
    vector_writer writer { via_writer };
    serialize(wire_pair { 3, 4 }, writer);

    std::vector<char> via_byte_sink;
    serialize(wire_pair { 3, 4 }, via_byte_sink);

    REQUIRE(via_writer == via_byte_sink);
}
