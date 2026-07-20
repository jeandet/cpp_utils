#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <numeric>
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

TEST_CASE("parallel_chunks_transform writes correct output offsets above threshold", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<int> input(count);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(count, -1);

    parallel_chunks_transform(input, output.data(), std::size_t { 1 },
        [](std::span<const int> chunk, int* out)
        {
            for (std::size_t i = 0; i < chunk.size(); ++i)
                out[i] = chunk[i] * 2;
        });

    for (std::size_t i = 0; i < count; ++i)
        REQUIRE(output[i] == static_cast<int>(i) * 2);
}

TEST_CASE("parallel_chunks_transform runs one serial call when below threshold", "[threading]")
{
    std::vector<int> input { 1, 2, 3, 4 };
    std::vector<int> output(4, -1);
    std::atomic<int> calls { 0 };

    parallel_chunks_transform(input, output.data(), std::size_t { 1'000'000 },
        [&](std::span<const int> chunk, int* out)
        {
            calls++;
            REQUIRE(chunk.size() == input.size());
            for (std::size_t i = 0; i < chunk.size(); ++i)
                out[i] = chunk[i];
        });

    REQUIRE(calls == 1);
    REQUIRE(output == input);
}
