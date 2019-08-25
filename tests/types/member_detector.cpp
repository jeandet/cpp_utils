#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types_detectors.h>


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
    REQUIRE(has_valid_member_object<TestStruc>);
    REQUIRE(!has_absent_member_object<TestStruc>);
    REQUIRE(!has_method_member_object<TestStruc>);
    
    REQUIRE(has_valid_member_object<DerivedTestStruc>);
    
    REQUIRE(!has_valid_member_object<PrivateDerivedTestStruc>);
}
