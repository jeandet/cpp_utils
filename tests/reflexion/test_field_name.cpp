#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <reflexion/field_name.hpp>

struct one_member
{
    int a;
};

struct point_like
{
    int x;
    int y;
    int z;
};

struct with_c_array_member
{
    int a;
    char buf[8];
    int b;
};

struct record_like
{
    std::uint32_t rNumDims;
    std::uint32_t rDimSizes;
    std::int32_t Encoding;
    std::int32_t Flags;
    std::int32_t rfuA;
};

static_assert(cpp_utils::reflexion::field_name<one_member, 0> == "a");

static_assert(cpp_utils::reflexion::field_name<point_like, 0> == "x");
static_assert(cpp_utils::reflexion::field_name<point_like, 1> == "y");
static_assert(cpp_utils::reflexion::field_name<point_like, 2> == "z");

static_assert(cpp_utils::reflexion::field_name<with_c_array_member, 0> == "a");
static_assert(cpp_utils::reflexion::field_name<with_c_array_member, 1> == "buf");
static_assert(cpp_utils::reflexion::field_name<with_c_array_member, 2> == "b");

static_assert(cpp_utils::reflexion::field_name<record_like, 0> == "rNumDims");
static_assert(cpp_utils::reflexion::field_name<record_like, 1> == "rDimSizes");
static_assert(cpp_utils::reflexion::field_name<record_like, 2> == "Encoding");
static_assert(cpp_utils::reflexion::field_name<record_like, 3> == "Flags");
static_assert(cpp_utils::reflexion::field_name<record_like, 4> == "rfuA");

TEST_CASE("field_name resolves each member's source name", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::field_name<point_like, 0> == "x");
    REQUIRE(cpp_utils::reflexion::field_name<point_like, 1> == "y");
    REQUIRE(cpp_utils::reflexion::field_name<point_like, 2> == "z");
}

TEST_CASE("field_name resolves a fixed-size C-array member's own name", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::field_name<with_c_array_member, 1> == "buf");
}
