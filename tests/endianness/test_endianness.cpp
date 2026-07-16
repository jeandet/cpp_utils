#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include "endianness/endianness.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>


TEST_CASE("Endianness scalars", "[decode]")
{
    using namespace cpp_utils::endianness;
    using target_endianness = std::conditional_t<host_is_big_endian,little_endian_t,big_endian_t>;
    REQUIRE(0x01 == decode<target_endianness, uint8_t>("\1"));
    REQUIRE(0x0102 == decode<target_endianness, uint16_t>("\1\2"));
    REQUIRE(0x01020304 == decode<target_endianness, uint32_t>("\1\2\3\4"));
    REQUIRE(0x0102030405060701 == decode<target_endianness, uint64_t>("\1\2\3\4\5\6\7\1"));
}

TEST_CASE("Endianness vector", "[decode]")
{
    using namespace cpp_utils::endianness;
    using target_endianness = std::conditional_t<host_is_big_endian,little_endian_t,big_endian_t>;
    std::vector<uint8_t> input{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    std::vector<uint8_t> output(16);
    decode_v<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint16_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({2, 1, 4,3,6,5,8,7,10,9,12,11,14,13,16,15}));
    decode_v<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint32_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({4,3,2,1,8,7,6,5,12,11,10,9,16,15,14,13}));
    decode_v<target_endianness>(input.data(),std::size(input),reinterpret_cast<uint64_t*>(output.data()));
    REQUIRE_THAT(output, Catch::Matchers::Equals<uint8_t>({8,7,6,5,4,3,2,1,16,15,14,13,12,11,10,9}));
}

TEST_CASE("Runtime-dispatched decode matches the compile-time decode for both byte orders", "[decode]")
{
    using namespace cpp_utils::endianness;
    REQUIRE(decode<little_endian_t, uint32_t>("\1\2\3\4")
        == decode<uint32_t>(Endianness::little, "\1\2\3\4"));
    REQUIRE(decode<big_endian_t, uint32_t>("\1\2\3\4")
        == decode<uint32_t>(Endianness::big, "\1\2\3\4"));
}

TEST_CASE("Runtime-dispatched encode matches the compile-time encode for both byte orders", "[encode]")
{
    using namespace cpp_utils::endianness;
    char runtime_little[4];
    char compiletime_little[4];
    encode<uint32_t>(Endianness::little, 0x01020304, runtime_little);
    encode<little_endian_t>(0x01020304u, compiletime_little);
    REQUIRE(std::equal(std::begin(runtime_little), std::end(runtime_little),
        std::begin(compiletime_little)));

    char runtime_big[4];
    char compiletime_big[4];
    encode<uint32_t>(Endianness::big, 0x01020304, runtime_big);
    encode<big_endian_t>(0x01020304u, compiletime_big);
    REQUIRE(std::equal(
        std::begin(runtime_big), std::end(runtime_big), std::begin(compiletime_big)));
}

TEST_CASE("Endianness vector at a misaligned buffer offset", "[decode]")
{
    // Regression test for a strict-aliasing/unaligned-access bug: decode_v used to
    // reinterpret_cast the source pointer straight to a wider type and dereference it, which is
    // undefined behavior when that pointer isn't naturally aligned for the target type (as here,
    // starting one byte into the buffer). Confirmed via -fsanitize=alignment before the fix.
    using namespace cpp_utils::endianness;
    using target_endianness = std::conditional_t<host_is_big_endian, little_endian_t, big_endian_t>;
    std::vector<uint8_t> input { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    uint32_t output[2];
    decode_v<target_endianness>(input.data() + 1, std::size_t { 8 }, output);
    REQUIRE(output[0] == 0x01020304);
    REQUIRE(output[1] == 0x05060708);
}
