#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <containers/nd_shape.hpp>
#include <vector>

using namespace cpp_utils::containers;

TEST_CASE("flat_index computes row-major (C-order, last dim fastest) offsets", "[containers]")
{
    const std::vector<std::size_t> shape { 2, 3 };
    REQUIRE(flat_index(std::vector<std::size_t> { 0, 0 }, shape) == 0);
    REQUIRE(flat_index(std::vector<std::size_t> { 0, 1 }, shape) == 1);
    REQUIRE(flat_index(std::vector<std::size_t> { 0, 2 }, shape) == 2);
    REQUIRE(flat_index(std::vector<std::size_t> { 1, 0 }, shape) == 3);
    REQUIRE(flat_index(std::vector<std::size_t> { 1, 2 }, shape) == 5);
}

TEST_CASE("flat_index computes column-major (Fortran-order, first dim fastest) offsets",
    "[containers]")
{
    const std::vector<std::size_t> shape { 2, 3 };
    REQUIRE(flat_index(std::vector<std::size_t> { 0, 0 }, shape, array_order::column_major) == 0);
    REQUIRE(flat_index(std::vector<std::size_t> { 1, 0 }, shape, array_order::column_major) == 1);
    REQUIRE(flat_index(std::vector<std::size_t> { 0, 1 }, shape, array_order::column_major) == 2);
    REQUIRE(flat_index(std::vector<std::size_t> { 1, 2 }, shape, array_order::column_major) == 5);
}

TEST_CASE("permute_axes transposes a 2D array", "[containers]")
{
    // row-major 2x3 [[1,2,3],[4,5,6]] -> transpose (perm {1,0}) -> row-major 3x2 [[1,4],[2,5],[3,6]]
    const std::vector<int> data { 1, 2, 3, 4, 5, 6 };
    const std::vector<std::size_t> shape { 2, 3 };
    const auto result = permute_axes(data, shape, std::vector<std::size_t> { 1, 0 });
    REQUIRE(result == std::vector<int> { 1, 4, 2, 5, 3, 6 });
}

TEST_CASE("permute_axes with the identity permutation is a no-op", "[containers]")
{
    const std::vector<int> data { 1, 2, 3, 4, 5, 6 };
    const std::vector<std::size_t> shape { 2, 3 };
    const auto result = permute_axes(data, shape, std::vector<std::size_t> { 0, 1 });
    REQUIRE(result == data);
}

TEST_CASE("permute_axes generalizes full-axis-reversal transpose to arbitrary dimension count",
    "[containers]")
{
    // shape (2,2,2): reversing all 3 axes (perm {2,1,0}) is CDFpp's majority-swap case.
    const std::vector<int> data { 0, 1, 2, 3, 4, 5, 6, 7 };
    const std::vector<std::size_t> shape { 2, 2, 2 };
    const auto result = permute_axes(data, shape, std::vector<std::size_t> { 2, 1, 0 });
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 2; ++j)
            for (std::size_t k = 0; k < 2; ++k)
            {
                const auto new_idx = flat_index(std::vector<std::size_t> { i, j, k }, shape);
                const auto old_idx = flat_index(std::vector<std::size_t> { k, j, i }, shape);
                REQUIRE(result[new_idx] == data[old_idx]);
            }
}
