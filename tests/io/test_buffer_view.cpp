#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <io/buffer_view.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

using namespace cpp_utils::io;

TEST_CASE("buffer_view wraps an in-memory buffer for reading", "[io]")
{
    std::vector<char> data { 'h', 'e', 'l', 'l', 'o' };

    buffer_view v { data };

    REQUIRE(v.is_valid());
    REQUIRE(v.size() == 5);
    REQUIRE(std::string(v.data(), v.size()) == "hello");
}

TEST_CASE("buffer_view::read copies an arbitrary offset/size range into a destination", "[io]")
{
    std::vector<char> data { 'h', 'e', 'l', 'l', 'o' };
    buffer_view v { data };

    char dest[3] {};
    v.read(dest, 1, 3);
    REQUIRE(std::string(dest, 3) == "ell");
}

TEST_CASE("buffer_view::view returns a zero-copy pointer at an arbitrary offset", "[io]")
{
    std::vector<char> data { 'h', 'e', 'l', 'l', 'o' };
    buffer_view v { data };

    REQUIRE(*v.view(4) == 'o');
}

TEST_CASE("a default-constructed buffer_view is invalid", "[io]")
{
    buffer_view v;
    REQUIRE_FALSE(v.is_valid());
}

TEST_CASE("buffer_view feeds serde::deserialize directly", "[io]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };
    std::vector<char> data { static_cast<char>(5), static_cast<char>(9) };

    auto value = cpp_utils::serde::deserialize<wire_pair>(buffer_view { data });
    REQUIRE(value.a == 5);
    REQUIRE(value.b == 9);
}
