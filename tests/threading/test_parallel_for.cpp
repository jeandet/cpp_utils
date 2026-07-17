#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <threading/parallel_for.hpp>
#include <vector>

using namespace cpp_utils::threading;

TEST_CASE("parallel_for calls f exactly once per index in [0, count)", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<std::atomic<int>> hits(count);
    parallel_for(count, [&](std::size_t i) { hits[i]++; });
    for (auto& hit : hits)
        REQUIRE(hit == 1);
}

TEST_CASE("parallel_for handles count 0 and 1 without touching the pool", "[threading]")
{
    int calls = 0;
    parallel_for(std::size_t { 0 }, [&](std::size_t) { calls++; });
    REQUIRE(calls == 0);

    std::size_t seen_index = 42;
    parallel_for(std::size_t { 1 }, [&](std::size_t i) { calls++; seen_index = i; });
    REQUIRE(calls == 1);
    REQUIRE(seen_index == 0);
}

TEST_CASE("parallel_for_each visits every element of a container", "[threading]")
{
    std::vector<int> items(200, 0);
    parallel_for_each(items, [](int& v) { v = 1; });
    for (auto v : items)
        REQUIRE(v == 1);
}
