#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <types/visitor.hpp>
#include <variant>

using namespace cpp_utils::types;

TEST_CASE("Visitor dispatches std::visit by alternative type", "[types]")
{
    std::variant<int, std::string> v = 42;

    auto describe = [](const auto& value)
    {
        return std::visit(Visitor { [](int i) { return std::string("int:") + std::to_string(i); },
                               [](const std::string& s) { return std::string("string:") + s; } },
            value);
    };

    REQUIRE(describe(v) == "int:42");

    v = std::string("hello");
    REQUIRE(describe(v) == "string:hello");
}
