#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include "endianness.hpp"
#include <cstdint>
#include <vector>


TEST_CASE("Endianness scalars", "[decode]")
{
    using namespace Endianness;
    using target_endianness = std::conditional_t<host_is_big_endian,little_endian_t,big_endian_t>;
    REQUIRE(0x01 == decode<target_endianness, uint8_t>("\1"));
    REQUIRE(0x0102 == decode<target_endianness, uint16_t>("\1\2"));
    REQUIRE(0x01020304 == decode<target_endianness, uint32_t>("\1\2\3\4"));
    REQUIRE(0x0102030405060701 == decode<target_endianness, uint64_t>("\1\2\3\4\5\6\7\1"));
}

TEST_CASE("Endianness vector", "[decode]")
{
    using namespace Endianness;
    using target_endianness = std::conditional_t<host_is_big_endian,little_endian_t,big_endian_t>;
    std::vector<uint8_t> input{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    std::vector<uint8_t> output(16);
    decode<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint16_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({2, 1, 4,3,6,5,8,7,10,9,12,11,14,13,16,15}));
    decode<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint32_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({4,3,2,1,8,7,6,5,12,11,10,9,16,15,14,13}));
    decode<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint64_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({8,7,6,5,4,3,2,1,16,15,14,13,12,11,10,9}));
}
