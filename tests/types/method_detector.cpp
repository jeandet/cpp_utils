#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types_detectors.h>

struct TestStruc
{
    int member;
    void valid(){};
};

struct DerivedTestStruc: TestStruc
{};

struct PrivateDerivedTestStruc: private TestStruc
{};

HAS_METHOD(valid)
HAS_METHOD(absent)
HAS_METHOD(member)



TEST_CASE( "Method detector", "[types]" ) {
    REQUIRE(has_valid_method<TestStruc>);
    REQUIRE(!has_absent_method<TestStruc>);
    REQUIRE(!has_member_method<TestStruc>);
    
    REQUIRE(has_valid_method<DerivedTestStruc>);
    
    REQUIRE(!has_valid_method<PrivateDerivedTestStruc>);
}

