#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <threading/parallel_chunks.hpp>
#include <vector>

using namespace cpp_utils::threading;

TEST_CASE("parallel_chunks_for_each mutates every element exactly once above threshold", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<int> items(count, 0);
    parallel_chunks_for_each(items, std::size_t { 1 },
        [](std::span<int> chunk)
        {
            for (auto& v : chunk)
                v += 1;
        });
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each runs one serial call when below threshold", "[threading]")
{
    std::vector<int> items(4, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(items, std::size_t { 1'000'000 },
        [&](std::span<int> chunk)
        {
            calls++;
            REQUIRE(chunk.size() == items.size());
            for (auto& v : chunk)
                v += 1;
        });
    REQUIRE(calls == 1);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each respects an explicit chunk_count", "[threading]")
{
    constexpr std::size_t count = 100;
    std::vector<int> items(count, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(
        items, std::size_t { 1 },
        [&](std::span<int> chunk)
        {
            calls++;
            for (auto& v : chunk)
                v += 1;
        },
        std::size_t { 4 });
    REQUIRE(calls == 4);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each: min_chunk_size 0 disables the threshold gate", "[threading]")
{
    constexpr std::size_t count = 4;
    std::vector<int> items(count, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(
        items, std::size_t { 0 },
        [&](std::span<int> chunk)
        {
            calls++;
            for (auto& v : chunk)
                v += 1;
        },
        std::size_t { 4 });
    REQUIRE(calls == 4);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each is a no-op for empty input", "[threading]")
{
    std::vector<int> items;
    int calls = 0;
    parallel_chunks_for_each(items, std::size_t { 1 }, [&](std::span<int>) { calls++; });
    REQUIRE(calls == 0);
}
