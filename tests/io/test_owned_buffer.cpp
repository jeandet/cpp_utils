#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <containers/no_init_vector.hpp>
#include <cstdint>
#include <io/owned_buffer.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

using namespace cpp_utils::io;
using cpp_utils::containers::no_init_vector;

namespace
{
no_init_vector<char> make_storage(std::initializer_list<char> chars)
{
    no_init_vector<char> v;
    v.assign(chars.begin(), chars.end());
    return v;
}
}

TEST_CASE("owned_buffer takes ownership of a moved-in buffer", "[io]")
{
    auto storage = make_storage({ 'h', 'e', 'l', 'l', 'o' });

    owned_buffer b { std::move(storage) };

    REQUIRE(b.is_valid());
    REQUIRE(b.size() == 5);
    REQUIRE(std::string(b.data(), b.size()) == "hello");
}

TEST_CASE("owned_buffer::read copies an arbitrary offset/size range into a destination", "[io]")
{
    owned_buffer b { make_storage({ 'h', 'e', 'l', 'l', 'o' }) };

    char dest[3] {};
    b.read(dest, 1, 3);
    REQUIRE(std::string(dest, 3) == "ell");
}

TEST_CASE("owned_buffer::view returns a zero-copy pointer at an arbitrary offset", "[io]")
{
    owned_buffer b { make_storage({ 'h', 'e', 'l', 'l', 'o' }) };
    REQUIRE(*b.view(4) == 'o');
}

TEST_CASE("a default-constructed owned_buffer is invalid", "[io]")
{
    owned_buffer b;
    REQUIRE_FALSE(b.is_valid());
}

TEST_CASE("owned_buffer feeds serde::deserialize directly", "[io]")
{
    struct wire_pair
    {
        uint8_t a;
        uint8_t b;
    };

    auto value = cpp_utils::serde::deserialize<wire_pair>(owned_buffer { make_storage({ 5, 9 }) });
    REQUIRE(value.a == 5);
    REQUIRE(value.b == 9);
}

TEST_CASE("owned_buffer_t genuinely moves an arbitrary contiguous storage, no copy", "[io]")
{
    std::vector<char> storage { 'h', 'e', 'l', 'l', 'o' };
    const char* original_ptr = storage.data();

    owned_buffer_t<std::vector<char>> b { std::move(storage) };

    REQUIRE(storage.data() == nullptr); // moved-from: source relinquished its buffer
    REQUIRE(b.data() == original_ptr); // same heap block landed in b, nothing was copied
    REQUIRE(b.size() == 5);
    REQUIRE(std::string(b.data(), b.size()) == "hello");
}
