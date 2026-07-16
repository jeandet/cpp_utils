#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <containers/nomap.hpp>

using namespace cpp_utils::containers;

TEST_CASE("nomap_node swap exchanges key and value between nodes", "[Containers][nomap]")
{
    nomap_node<int, int> a { 10, 1 };
    nomap_node<int, int> b { 20, 2 };

    using std::swap;
    swap(a, b);

    REQUIRE(a.key() == 20);
    REQUIRE(a.mapped() == 2);
    REQUIRE(b.key() == 10);
    REQUIRE(b.mapped() == 1);
}

TEST_CASE("nomap erase(first, last) can erase a range through end()", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;
    m[2] = 20;

    auto it = m.erase(m.cbegin(), m.cend());

    REQUIRE(m.empty());
    REQUIRE(it == m.end());
}

TEST_CASE("nomap erase(first, last) removes exactly the requested count", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;
    m[2] = 20;
    m[3] = 30;
    m[4] = 40;

    m.erase(m.cbegin(), m.cbegin() + 2);

    REQUIRE(m.size() == 2);
}
