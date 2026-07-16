#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <types/concepts.hpp>
#include <vector>
#include <memory>

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
}
