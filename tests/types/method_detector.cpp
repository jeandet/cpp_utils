#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types/detectors.hpp>

struct TestStruc
{
    int member;
    void valid(){};
    void overload_method(){};
    void overload_method(int){};
    void overload_method(double){};  
    void overload_method(double, int){}; 
};

struct DerivedTestStruc: TestStruc
{};

struct PrivateDerivedTestStruc: private TestStruc
{};

HAS_METHOD(has_valid_method, valid)
HAS_METHOD(has_absent_method, absent)
HAS_METHOD(has_member_method, member)
HAS_METHOD(has_overload_void_method, overload_method)
HAS_METHOD(has_overload_int_method, overload_method, int)
HAS_METHOD(has_overload_double_method, overload_method, double)
HAS_METHOD(has_overload_double_int_method, overload_method, double, int)



TEST_CASE( "Method detector", "[types]" ) {
    REQUIRE(has_valid_method_v<TestStruc>);
    REQUIRE(has_overload_void_method_v<TestStruc>);
    REQUIRE(has_overload_int_method_v<TestStruc>);
    REQUIRE(has_overload_double_method_v<TestStruc>);
    REQUIRE(has_overload_double_int_method_v<TestStruc>);
    REQUIRE(!has_absent_method_v<TestStruc>);
    REQUIRE(!has_member_method_v<TestStruc>);
    
    REQUIRE(has_valid_method_v<DerivedTestStruc>);
    
    REQUIRE(!has_valid_method_v<PrivateDerivedTestStruc>);
}

