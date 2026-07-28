#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <reflexion/enum_name.hpp>
#include <reflexion/field_name.hpp>
#include <reflexion/reflection.hpp>

/*
 * Regression test: enum_name.hpp and field_name.hpp both declare a nested
 * cpp_utils::reflexion::details namespace. reflection.hpp's own
 * count_members<T> resolves an unqualified `details::MemberCounter<T>()` from
 * inside namespace cpp_utils::reflexion, expecting ordinary unqualified
 * lookup to fall through to the global ::details namespace it defines at
 * file scope. If either sibling header's inner namespace were ever renamed
 * back to `details`, unqualified lookup would instead resolve to that
 * *closer* nested namespace and count_members would fail to compile - this
 * TU is what would catch it, since neither header's own tests include
 * reflection.hpp alongside them.
 */

struct seven_members
{
    int a, b, c, d, e, f, g;
};

enum class combined_test_e
{
    Foo = 0,
    Bar = 1,
};

static_assert(cpp_utils::reflexion::count_members<seven_members> == 7);
static_assert(cpp_utils::reflexion::field_name<seven_members, 0> == "a");
static_assert(cpp_utils::reflexion::enum_name(combined_test_e::Foo) == "Foo");

TEST_CASE(
    "count_members, field_name and enum_name coexist in the same translation unit", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::count_members<seven_members> == 7);
    REQUIRE(cpp_utils::reflexion::field_name<seven_members, 6> == "g");
    REQUIRE(cpp_utils::reflexion::enum_name(combined_test_e::Bar) == "Bar");
}
