
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <types/detectors.hpp>
#include <map>
#include <iostream>


struct TestStruc
{
    using valid = int;
    using valid_ref = int&;
    using valid_const_ref = const int&;
    int value;
    void method(){}
};

struct DerivedTestStruc: TestStruc
{};

struct PrivateDerivedTestStruc: private TestStruc
{};

HAS_TYPE(valid)
HAS_TYPE(valid_ref)
HAS_TYPE(valid_const_ref)
HAS_TYPE(value)
HAS_TYPE(absent)
HAS_TYPE(method)

IS_T(is_TestStruc, TestStruc)
IS_TEMPLATE_T(is_std_vector, std::vector)



TEST_CASE( "Member detector", "[types]" ) {
    REQUIRE(has_valid_type_v<TestStruc>);
    REQUIRE(has_valid_ref_type_v<TestStruc>);
    REQUIRE(has_valid_const_ref_type_v<TestStruc>);
    REQUIRE(!has_value_type_v<TestStruc>);
    REQUIRE(!has_absent_type_v<TestStruc>);
    REQUIRE(!has_method_type_v<TestStruc>);
    
    REQUIRE(has_valid_type_v<DerivedTestStruc>);
    
    REQUIRE(!has_valid_type_v<PrivateDerivedTestStruc>);
    REQUIRE(is_TestStruc_v<TestStruc>);
    REQUIRE(!is_TestStruc_v<int>);
    REQUIRE(is_std_vector_v<std::vector<int>>);
    REQUIRE(!is_std_vector_v<int>);

    using namespace cpp_utils::types::detectors;
    REQUIRE(std::is_same_v<is_any_of_t<int, char, double>,std::false_type>);
    REQUIRE(std::is_same_v<is_any_of_t<int, char,int, double>,std::true_type>);
    REQUIRE(!is_any_of_v<int, char, double>);
    REQUIRE(is_any_of_v<int, char,int, double>);
}
