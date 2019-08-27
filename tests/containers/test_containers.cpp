#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <array>
#include <catch2/catch.hpp>
#include <containers.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>


using namespace cpp_utils::containers;

TEST_CASE("Containers", "[Containers]")
{
#define TEST_CONTAINER(type)                                                                       \
    {                                                                                              \
        std::type<double> type { 1., 2., 3. };                                                     \
        REQUIRE(contains(type, 1.));                                                               \
        REQUIRE(!contains(type, 10.));                                                             \
        REQUIRE(index_of(type, 2.) == 1);                                                          \
    }

    TEST_CONTAINER(list)
    TEST_CONTAINER(vector)
    TEST_CONTAINER(deque)
    TEST_CONTAINER(forward_list)

    std::set<double> set { 1., 2., 3. };
    std::array<double, 3> arr { 1., 2., 3. };
    std::map<char, double> map { { 'a', 1. }, { 'b', 2. }, { 'c', 3. } };

    REQUIRE(contains(arr, 1.));
    REQUIRE(!contains(arr, 10.));
    REQUIRE(index_of(arr, 2.) == 1);

    REQUIRE(contains(set, 1.));
    REQUIRE(!contains(set, 10.));
    // REQUIRE(index_of(set, 2.)==1);

    REQUIRE(contains(map, 'a'));
    REQUIRE(!contains(map, 'z'));
    // REQUIRE(index_of(list, 'b')==1);
}
