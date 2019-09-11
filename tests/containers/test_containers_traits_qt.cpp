#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <array>
#include <catch2/catch.hpp>
#include <cpp_utils_qt/cpp_utils_qt.hpp>
#include <containers/traits.hpp>
#include <QVector>


using namespace cpp_utils::containers;

TEST_CASE("Qt Containers traits", "[Containers]")
{
#define TEST_IS_CONTAINER(type, ...)                                                               \
    {                                                                                              \
        REQUIRE(is_container_v<type<double, ##__VA_ARGS__>>);                                 \
        REQUIRE(_is_container<type<double, ##__VA_ARGS__>>());                                \
    }

    TEST_IS_CONTAINER(QVector)
}
