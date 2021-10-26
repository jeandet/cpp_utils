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

struct TestBroadcast
{
    static int result;
    void add(int value) const { result += value; }
    void increment(int& value) { value++; }
};

int TestBroadcast::result = 0;

using namespace cpp_utils::containers;

TEST_CASE("Containers algorithms", "[Containers]")
{
#define ARG(...) __VA_ARGS__

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
        REQUIRE(index_of(type, 1111.) == static_cast<int>(std::size(type)));                       \
    }

#define TEST_SPLIT_T(value_type, value, expected_value_type, expected_value, sep)                  \
    {                                                                                              \
        value_type test value;                                                                     \
        expected_value_type res;                                                                   \
        expected_value_type expected expected_value;                                               \
        split(test, res, sep);                                                                     \
        REQUIRE(std::size(res) == std::size(expected));                                            \
        REQUIRE(res == expected);                                                                  \
    }

#define TEST_SPLIT(value, expected_value)                                                          \
    TEST_SPLIT_T(                                                                                  \
        ARG(std::string), ARG(value), ARG(std::vector<std::string>), ARG(expected_value), '/')


#define TEST_JOIN_T(value_type, value, expected_value_type, expected_value, sep)                   \
    {                                                                                              \
        value_type test value;                                                                     \
        expected_value_type expected expected_value;                                               \
        auto res = join(test, sep);                                                                \
        REQUIRE(std::size(res) == std::size(expected));                                            \
        REQUIRE(res == expected);                                                                  \
    }


#define TEST_JOIN(value, expected_value)                                                           \
    TEST_JOIN_T(                                                                                   \
        ARG(std::vector<std::string>), ARG(value), ARG(std::string), ARG(expected_value), '/')

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


    TEST_SPLIT({ "/part1/part2/part3" }, ARG({ "part1", "part2", "part3" }));
    TEST_SPLIT({ "" }, ARG({}));
    TEST_SPLIT({ "///////" }, ARG({}));
    TEST_SPLIT({ "/path/" }, ARG({ "path" }));
    TEST_SPLIT({ "/path" }, ARG({ "path" }));
    TEST_SPLIT({ "path/" }, ARG({ "path" }));
    TEST_SPLIT({ "path" }, ARG({ "path" }));
    TEST_SPLIT({ "path1/path2" }, ARG({ "path1", "path2" }));

    TEST_SPLIT_T(ARG(std::vector<int>), ARG({ 1, 2, 3, 4, 5, 6 }), ARG(std::list<std::vector<int>>),
        ARG({ { 1, 2, 3 }, { 5, 6 } }), 4)

    TEST_JOIN(ARG({ "part1", "part2", "part3" }), { "part1/part2/part3" })
    TEST_JOIN(ARG({}), { "" })
    TEST_JOIN(ARG({ "part" }), { "part" })
    TEST_JOIN(ARG({ "part1", "part2" }), { "part1/part2" })

    TEST_JOIN_T(ARG(std::list<std::vector<int>>), ARG({ { 1, 2, 3 }, { 5, 6 } }),
        ARG(std::vector<int>), ARG({ 1, 2, 3, 4, 5, 6 }), 4)


    REQUIRE(*min(std::vector { 1, 2, 3 }) == 1);
    REQUIRE(*max(std::vector { 1, 2, 3 }) == 3);
    REQUIRE(min(std::vector<int> {}) == std::nullopt);
    REQUIRE(max(std::vector<int> {}) == std::nullopt);

    std::vector<TestBroadcast> c(10);
    broadcast(c, &TestBroadcast::add, 1);
    REQUIRE(TestBroadcast::result == 10);
    int value = 0;
    broadcast(c, &TestBroadcast::increment, value);
    REQUIRE(value == 10);
}
