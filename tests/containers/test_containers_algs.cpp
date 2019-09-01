#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <array>
#include <catch2/catch.hpp>
#include <containers/algorithms.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace std
{
template <typename T>
std::size_t size(const std::forward_list<T>& list)
{
    return std::distance(std::cbegin(list), std::cend(list));
}
}


using namespace cpp_utils::containers;

TEST_CASE("Containers algorithms", "[Containers]")
{
#define SINGLE_ARG(...) __VA_ARGS__
    
#define associative_init                                                                           \
    {                                                                                              \
        { 1., 'a' }, { 2., 'b' }, { 3., 'c' }                                                      \
    }
#define default_init                                                                               \
    {                                                                                              \
        1., 2., 3.                                                                                 \
    }
#define TEST_CONTAINER_CONTAINS(type, init, ...)                                                   \
    {                                                                                              \
        std::type<double, ##__VA_ARGS__> type init;                                                \
        REQUIRE(contains(type, 1.));                                                               \
        REQUIRE(!contains(type, 10.));                                                             \
    }

#define TEST_CONTAINER_INDEX_OF(type, init, ...)                                                   \
    {                                                                                              \
        std::type<double, ##__VA_ARGS__> type init;                                                \
        REQUIRE(index_of(type, 2.) == 1);                                                          \
        REQUIRE(index_of(type, 1111.) == std::size(type));                                         \
    }

    
#define TEST_SPLIT(value, expected_value)\
    {\
        std::string test value;\
        std::vector<std::string> res;\
        decltype(res) expected expected_value;\
        split(test, res, '/');\
        REQUIRE(std::size(res) == std::size(expected));\
        REQUIRE(res == expected);\
    }

    TEST_CONTAINER_CONTAINS(list, default_init)
    TEST_CONTAINER_CONTAINS(vector, default_init)
    TEST_CONTAINER_CONTAINS(deque, default_init)
    TEST_CONTAINER_CONTAINS(forward_list, default_init)
    TEST_CONTAINER_CONTAINS(array, default_init, 3)
    TEST_CONTAINER_CONTAINS(set, default_init)
    TEST_CONTAINER_INDEX_OF(list, default_init)
    TEST_CONTAINER_INDEX_OF(vector, default_init)
    TEST_CONTAINER_INDEX_OF(deque, default_init)
    TEST_CONTAINER_INDEX_OF(forward_list, default_init)
    TEST_CONTAINER_INDEX_OF(array, default_init, 3)
    TEST_CONTAINER_INDEX_OF(set, default_init)

    TEST_CONTAINER_CONTAINS(map, associative_init, char)

    
    TEST_SPLIT({ "/part1/part2/part3" }, SINGLE_ARG({ "part1", "part2", "part3" }));
    TEST_SPLIT({ "" }, SINGLE_ARG({}));
    TEST_SPLIT({ "///////" }, SINGLE_ARG({}));
    TEST_SPLIT({ "/path/" }, SINGLE_ARG({ "path"}));
    TEST_SPLIT({ "/path" }, SINGLE_ARG({ "path"}));
    TEST_SPLIT({ "path/" }, SINGLE_ARG({ "path"}));
    TEST_SPLIT({ "path" }, SINGLE_ARG({ "path"}));
    TEST_SPLIT({ "path1/path2" }, SINGLE_ARG({ "path1", "path2"}));
    
}
