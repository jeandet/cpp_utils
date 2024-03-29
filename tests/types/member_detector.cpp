#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <types/detectors.hpp>
#include <map>
#include <iostream>


struct TestStruc
{
    int valid;
    void method(){};
};

struct DerivedTestStruc: TestStruc
{};

struct PrivateDerivedTestStruc: private TestStruc
{};

HAS_MEMBER(valid)
HAS_MEMBER(absent)
HAS_MEMBER(method)



TEST_CASE( "Member detector", "[types]" ) {
    REQUIRE(has_valid_member_object_v<TestStruc>);
    REQUIRE(!has_absent_member_object_v<TestStruc>);
    REQUIRE(!has_method_member_object_v<TestStruc>);
    
    REQUIRE(has_valid_member_object_v<DerivedTestStruc>);
    
    REQUIRE(!has_valid_member_object_v<PrivateDerivedTestStruc>);
}
