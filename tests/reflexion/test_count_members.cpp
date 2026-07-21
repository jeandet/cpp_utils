#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <reflexion/reflection.hpp>

struct one_member
{
    int a;
};

struct five_members
{
    int a, b, c, d, e;
};

struct thirty_one_members
{
    int a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,
        a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30;
};

struct with_c_array_member
{
    int a;
    char buf[8];
    int b;
};

static_assert(cpp_utils::reflexion::count_members<one_member> == 1);
static_assert(cpp_utils::reflexion::count_members<five_members> == 5);
static_assert(cpp_utils::reflexion::count_members<thirty_one_members> == 31);
static_assert(cpp_utils::reflexion::count_members<with_c_array_member> == 3);

static_assert(cpp_utils::reflexion::composite_size<with_c_array_member>()
    == 2 * sizeof(int) + sizeof(with_c_array_member::buf));

TEST_CASE("count_members compile-time checks", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::count_members<one_member> == 1);
    REQUIRE(cpp_utils::reflexion::count_members<with_c_array_member> == 3);
}

TEST_CASE("composite_size accounts for fixed-size C-array members", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::composite_size<with_c_array_member>()
        == 2 * sizeof(int) + sizeof(with_c_array_member::buf));
}
