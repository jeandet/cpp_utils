#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <types/pointers.hpp>
#include <types/detectors.hpp>


TEST_CASE("Smart pointers", "[types]")
{
    using namespace cpp_utils::types::pointers;
    
    using unique_p = std::unique_ptr<double>;
    using shared_p = std::shared_ptr<double>;
    using weak_p = std::weak_ptr<double>;
    
    REQUIRE(is_std_unique_ptr<unique_p>::value);
    REQUIRE(!is_std_unique_ptr<shared_p>::value);
    REQUIRE(!is_std_unique_ptr<weak_p>::value);
    REQUIRE(!is_std_unique_ptr<int>::value);
    REQUIRE(!is_std_unique_ptr<int*>::value);

    REQUIRE(is_std_shared_ptr<shared_p>::value);
    REQUIRE(!is_std_shared_ptr<unique_p>::value);
    REQUIRE(!is_std_shared_ptr<weak_p>::value);
    REQUIRE(!is_std_shared_ptr<int>::value);
    REQUIRE(!is_std_shared_ptr<int*>::value);
    
    REQUIRE(is_std_weak_ptr<weak_p>::value);
    REQUIRE(!is_std_weak_ptr<shared_p>::value);
    REQUIRE(!is_std_weak_ptr<unique_p>::value);
    REQUIRE(!is_std_weak_ptr<int>::value);
    REQUIRE(!is_std_weak_ptr<int*>::value);

    REQUIRE(is_smart_ptr<unique_p>::value);
    REQUIRE(is_smart_ptr<shared_p>::value);
    REQUIRE(is_smart_ptr<weak_p>::value);

    REQUIRE(!is_smart_ptr<double>::value);
    REQUIRE(!is_smart_ptr<double*>::value);
    
    REQUIRE(std::is_same_v<decltype(to_value(std::declval<std::unique_ptr<double>>())),double>);
    REQUIRE(std::is_same_v<decltype(to_value(std::declval<double*>())),double>);
    REQUIRE(std::is_same_v<decltype(to_value(std::declval<double>())),double>);
    
    REQUIRE(std::is_same_v<decltype(to_value_ref(std::declval<std::unique_ptr<double>>())),double&>);
    REQUIRE(std::is_same_v<decltype(to_value_ref(std::declval<double*>())),double&>);
    static double d;
    REQUIRE(std::is_same_v<decltype(to_value_ref(d)),double&>);
}
