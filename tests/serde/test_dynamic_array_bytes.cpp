#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("dynamic_array_bytes sized from the parent record itself", "[serde]")
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
    std::vector<uint8_t> buffer { 4, 0, 1, 0, 2, 0 };
    auto s = cpp_utils::serde::deserialize<with_table>(buffer);
    REQUIRE(s.values.size() == 2);
    REQUIRE(s.values[0] == 1);
    REQUIRE(s.values[1] == 2);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 6);
    REQUIRE(sink == buffer);
}

TEST_CASE("dynamic_array_bytes sized from an external context (CDFpp's rNumDims case)", "[serde]")
{
    struct dims_context
    {
        int32_t num_dims;
    };
    struct rvdr_like
    {
        cpp_utils::serde::dynamic_array_bytes<0, int32_t> dim_varys;

        std::size_t field_size(const cpp_utils::serde::dynamic_array_bytes<0, int32_t>&,
            const dims_context& ctx) const
        {
            return static_cast<std::size_t>(ctx.num_dims) * sizeof(int32_t);
        }
    };
    std::vector<uint8_t> buffer { 1, 0, 0, 0, 0, 0, 0, 0 };
    dims_context ctx { 2 };
    auto s = cpp_utils::serde::deserialize<rvdr_like>(buffer, 0, ctx);
    REQUIRE(s.dim_varys.size() == 2);
    REQUIRE(s.dim_varys[0] == 1);
    REQUIRE(s.dim_varys[1] == 0);
}

TEST_CASE("dynamic_array_bytes in a non-trailing struct position round-trips", "[serde]")
{
    struct with_leading_and_trailing
    {
        uint16_t before;
        uint16_t count_bytes;
        cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;
        uint16_t after;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    std::vector<uint8_t> buffer { 9, 0, 4, 0, 1, 0, 2, 0, 7, 0 };
    auto s = cpp_utils::serde::deserialize<with_leading_and_trailing>(buffer);
    REQUIRE(s.before == 9);
    REQUIRE(s.count_bytes == 4);
    REQUIRE(s.values.size() == 2);
    REQUIRE(s.values[0] == 1);
    REQUIRE(s.values[1] == 2);
    REQUIRE(s.after == 7);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 10);
    REQUIRE(sink == buffer);
}
