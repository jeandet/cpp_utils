
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types/detectors.hpp>
#include <map>
#include <iostream>


struct TestStruc
{
    using valid = int;
    int value;
    void method(){};
};

struct DerivedTestStruc: TestStruc
{};

struct PrivateDerivedTestStruc: private TestStruc
{};

HAS_TYPE(valid)
HAS_TYPE(value)
HAS_TYPE(absent)
HAS_TYPE(method)



TEST_CASE( "Member detector", "[types]" ) {
    REQUIRE(has_valid_type_v<TestStruc>);
    REQUIRE(!has_value_type_v<TestStruc>);
    REQUIRE(!has_absent_type_v<TestStruc>);
    REQUIRE(!has_method_type_v<TestStruc>);
    
    REQUIRE(has_valid_type_v<DerivedTestStruc>);
    
    REQUIRE(!has_valid_type_v<PrivateDerivedTestStruc>);
}
