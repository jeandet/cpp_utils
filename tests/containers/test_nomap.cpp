#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <containers/nomap.hpp>
#include <stdexcept>
#include <string>

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

TEST_CASE("nomap erase(first, last) removes exactly the targeted keys, not substitutes from the tail",
    "[Containers][nomap]")
{
    nomap<int, int> m;
    m[0] = 0;
    m[1] = 1;
    m[2] = 2;
    m[3] = 3;
    m[4] = 4;

    // erase the middle range [1, 3) -- keys 1 and 2
    m.erase(m.begin() + 1, m.begin() + 3);

    REQUIRE(m.size() == 3);
    REQUIRE(m.find(1) == m.end());
    REQUIRE(m.find(2) == m.end());
    REQUIRE(m.find(0) != m.end());
    REQUIRE(m.find(3) != m.end());
    REQUIRE(m.find(4) != m.end());
}

TEST_CASE("nomap operator[] with an rvalue key overwrites an existing key", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10; // rvalue-key overload, key not found -> insert
    m[1] = 20; // rvalue-key overload, key found -> overwrite in place

    REQUIRE(m.size() == 1);
    REQUIRE(m[1] == 20);
}

TEST_CASE("nomap operator[] with an lvalue key inserts then overwrites", "[Containers][nomap]")
{
    nomap<int, int> m;
    int key = 1;
    m[key] = 10; // lvalue-key overload, key not found -> insert
    m[key] = 20; // lvalue-key overload, key found -> overwrite in place

    REQUIRE(m.size() == 1);
    REQUIRE(m[key] == 20);
}

TEST_CASE("nomap find returns end() for a missing key", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;

    REQUIRE(m.find(2) == m.end());
    REQUIRE(m.find(1) != m.end());
}

TEST_CASE(
    "nomap at() returns the value for a present key and throws out_of_range for a missing one",
    "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;

    REQUIRE(m.at(1) == 10);
    REQUIRE_THROWS_AS(m.at(2), std::out_of_range);

    const auto& cm = m;
    REQUIRE(cm.at(1) == 10);
    REQUIRE_THROWS_AS(cm.at(2), std::out_of_range);
}

TEST_CASE("nomap extract(key) removes and returns the matching element", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;
    m[2] = 20;

    auto node = m.extract(1);

    REQUIRE_FALSE(node.empty());
    REQUIRE(node.key() == 1);
    REQUIRE(node.mapped() == 10);
    REQUIRE(m.size() == 1);
    REQUIRE(m.find(1) == m.end());
}

TEST_CASE(
    "nomap extract(key) on a missing key returns an empty node and leaves the map untouched",
    "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;

    auto node = m.extract(2);

    REQUIRE(node.empty());
    REQUIRE(m.size() == 1);
}

TEST_CASE("nomap erase(iterator) removes the pointed-to element", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;
    m[2] = 20;

    auto next = m.erase(m.find(1));

    REQUIRE(m.size() == 1);
    REQUIRE(m.find(1) == m.end());
    REQUIRE(next->first == 2);
}

TEST_CASE("nomap erase(iterator) on end() is a no-op that returns end()", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;

    auto next = m.erase(m.end());

    REQUIRE(m.size() == 1);
    REQUIRE(next == m.end());
}

TEST_CASE("nomap erase(const_iterator) removes the pointed-to element", "[Containers][nomap]")
{
    nomap<std::string, int> m;
    m["a"] = 10;
    m["b"] = 20;

    nomap<std::string, int>::const_iterator it = m.find("a");
    auto next = m.erase(it);

    REQUIRE(m.size() == 1);
    REQUIRE(m.find("a") == m.end());
    REQUIRE(next->first == "b");
}

TEST_CASE("nomap count() reports key presence", "[Containers][nomap]")
{
    nomap<int, int> m;
    m[1] = 10;

    REQUIRE(m.count(1) == 1);
    REQUIRE(m.count(2) == 0);
}

TEST_CASE("nomap operator== compares equal maps as equal", "[Containers][nomap]")
{
    nomap<int, int> a;
    a[1] = 10;
    a[2] = 20;
    nomap<int, int> b;
    b[1] = 10;
    b[2] = 20;

    REQUIRE(a == b);
    REQUIRE_FALSE(a != b);
}

TEST_CASE(
    "nomap operator== is not fooled by a superset -- a subset must not compare equal",
    "[Containers][nomap]")
{
    nomap<int, int> small_map;
    small_map[1] = 10;
    nomap<int, int> big_map;
    big_map[1] = 10;
    big_map[2] = 20;

    REQUIRE_FALSE(small_map == big_map);
    REQUIRE_FALSE(big_map == small_map);
    REQUIRE(small_map != big_map);
}
