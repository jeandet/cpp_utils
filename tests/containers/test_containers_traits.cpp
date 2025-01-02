#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <containers/traits.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>


using namespace cpp_utils::containers;

TEST_CASE("Containers traits", "[Containers]")
{
#define TEST_IS_CONTAINER(type, ...)                                                               \
    {                                                                                              \
        REQUIRE(is_container_v<std::type<double, ##__VA_ARGS__ > >);                               \
        REQUIRE(_is_container<std::type<double, ##__VA_ARGS__ > >());                              \
    }

    TEST_IS_CONTAINER(list)
    TEST_IS_CONTAINER(vector)
    TEST_IS_CONTAINER(deque)
    TEST_IS_CONTAINER(forward_list)
    TEST_IS_CONTAINER(array, 3)
    TEST_IS_CONTAINER(set)
    TEST_IS_CONTAINER(map, double)
}
