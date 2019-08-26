#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types/detectors.hpp>

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
    REQUIRE(has_valid_method_v<TestStruc>);
    REQUIRE(!has_absent_method_v<TestStruc>);
    REQUIRE(!has_member_method_v<TestStruc>);
    
    REQUIRE(has_valid_method_v<DerivedTestStruc>);
    
    REQUIRE(!has_valid_method_v<PrivateDerivedTestStruc>);
}

