#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types_detectors.h>
#include <memory>




TEST_CASE( "Smart pointers", "[types]" ) {
    REQUIRE(is_smart_ptr<std::shared_ptr<double>>::value);
    REQUIRE(is_smart_ptr<std::unique_ptr<double>>::value);
    
    REQUIRE(!is_smart_ptr<double>::value);
    REQUIRE(!is_smart_ptr<double*>::value);
}
