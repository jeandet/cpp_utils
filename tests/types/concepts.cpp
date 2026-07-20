#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <io/memory_mapped_file.hpp>
#include <types/concepts.hpp>
#include <vector>
#include <memory>
#include <array>
#include <list>
#include <span>

using namespace cpp_utils::types::concepts;
bool test_function(pointer_to_contiguous_memory auto)
{
    return true;
}

bool test_function(auto)
{
    return false;
}


TEST_CASE("Concepts", "[types]")
{
    double d;
    REQUIRE(test_function(&d));
    REQUIRE(!test_function(d));
    REQUIRE(!test_function(std::vector { 1.0, 2.0, 3.0 }));
    REQUIRE(test_function(std::data(std::vector { 1.0, 2.0, 3.0 })));
    REQUIRE(!test_function(std::make_unique<double>()));

    REQUIRE(random_access_buffer<cpp_utils::io::memory_mapped_file>);
    REQUIRE(!random_access_buffer<int>);
    REQUIRE(!random_access_buffer<std::vector<int>>);

    REQUIRE(contiguous_sized_range<std::vector<int>>);
    REQUIRE(contiguous_sized_range<std::array<int, 4>>);
    REQUIRE(!contiguous_sized_range<std::list<int>>);
    REQUIRE(!contiguous_sized_range<int>);

    auto good_for_each = [](std::span<int>) {};
    auto bad_arity = [](int) {};
    auto bad_element_type = [](std::span<double>) {};
    REQUIRE((chunk_for_each_callback<decltype(good_for_each), std::vector<int>>));
    REQUIRE(!(chunk_for_each_callback<decltype(bad_arity), std::vector<int>>));
    REQUIRE(!(chunk_for_each_callback<decltype(bad_element_type), std::vector<int>>));

    auto good_transform = [](std::span<const int>, int*) {};
    auto bad_output_type = [](std::span<const int>, double*) {};
    REQUIRE((chunk_transform_callback<decltype(good_transform), std::vector<int>, int*>));
    REQUIRE(!(chunk_transform_callback<decltype(bad_output_type), std::vector<int>, int*>));
}
