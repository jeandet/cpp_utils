#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <reflexion/enum_name.hpp>

namespace ns
{

enum class sequential_e
{
    a = 0,
    b = 1,
    c = 2
};

enum class sparse_e : int32_t
{
    Alpha = 1,
    Gamma = 4,
    Zeta = 17,
};

enum class with_negative_e : int32_t
{
    Sentinel = -1,
    Zero = 0,
    One = 1,
};

}

enum class global_e
{
    First = 0,
    Second = 1,
};

static_assert(cpp_utils::reflexion::enum_name(ns::sequential_e::a) == "a");
static_assert(cpp_utils::reflexion::enum_name(ns::sequential_e::b) == "b");
static_assert(cpp_utils::reflexion::enum_name(ns::sequential_e::c) == "c");

static_assert(cpp_utils::reflexion::enum_name(ns::sparse_e::Alpha) == "Alpha");
static_assert(cpp_utils::reflexion::enum_name(ns::sparse_e::Gamma) == "Gamma");
static_assert(cpp_utils::reflexion::enum_name(ns::sparse_e::Zeta) == "Zeta");
static_assert(cpp_utils::reflexion::enum_name(static_cast<ns::sparse_e>(2)).empty());
static_assert(cpp_utils::reflexion::enum_name(static_cast<ns::sparse_e>(0)).empty());

static_assert(cpp_utils::reflexion::enum_name(ns::with_negative_e::Sentinel) == "Sentinel");
static_assert(cpp_utils::reflexion::enum_name(ns::with_negative_e::Zero) == "Zero");
static_assert(cpp_utils::reflexion::enum_name(ns::with_negative_e::One) == "One");

static_assert(cpp_utils::reflexion::enum_name(global_e::First) == "First");
static_assert(cpp_utils::reflexion::enum_name(global_e::Second) == "Second");

TEST_CASE("enum_name resolves each enumerator's own source name", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sequential_e::a) == "a");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sequential_e::b) == "b");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sequential_e::c) == "c");
}

TEST_CASE("enum_name resolves sparse enumerators and rejects the gaps", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sparse_e::Alpha) == "Alpha");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sparse_e::Gamma) == "Gamma");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::sparse_e::Zeta) == "Zeta");
    REQUIRE(cpp_utils::reflexion::enum_name(static_cast<ns::sparse_e>(2)).empty());
    REQUIRE(cpp_utils::reflexion::enum_name(static_cast<ns::sparse_e>(255)).empty());
}

TEST_CASE("enum_name handles negative enumerator values", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::enum_name(ns::with_negative_e::Sentinel) == "Sentinel");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::with_negative_e::Zero) == "Zero");
    REQUIRE(cpp_utils::reflexion::enum_name(ns::with_negative_e::One) == "One");
}

TEST_CASE("enum_name works for an enum with no enclosing namespace", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::enum_name(global_e::First) == "First");
    REQUIRE(cpp_utils::reflexion::enum_name(global_e::Second) == "Second");
}
