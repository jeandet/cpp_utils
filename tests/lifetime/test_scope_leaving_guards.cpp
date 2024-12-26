#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "lifetime/scope_leaving_guards.hpp"
#include <catch2/catch_test_macros.hpp>

struct TestType
{
    int count;
    TestType() { count = 1; }
    ~TestType() { }
};

void callback(TestType* obj)
{
    obj->count--;
}

TEST_CASE("Scope leaving guards", "[strings]")
{
    using namespace cpp_utils::lifetime;
#if __cplusplus >= 202300
    REQUIRE(sizeof(scope_leaving_guard<TestType, callback>) == sizeof(void*));
#elif __cplusplus >= 20200

    REQUIRE(sizeof(scope_leaving_guard<TestType, callback>) <= 2 * sizeof(void*));
#endif
    TestType t;
    {
        auto guard = scope_leaving_guard<TestType, callback>(&t);
        REQUIRE(t.count == 1);
    }
    REQUIRE(t.count == 0);
}
