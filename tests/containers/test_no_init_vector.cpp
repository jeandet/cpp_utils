#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <containers/no_init_vector.hpp>
#include <string>

using namespace cpp_utils::containers;

TEST_CASE("no_init_vector of a trivial type stores and retrieves values", "[Containers][no_init_vector]")
{
    no_init_vector<int> v;
    v.resize(16);
    for (std::size_t i = 0; i < v.size(); i++)
        v[i] = static_cast<int>(i);

    for (std::size_t i = 0; i < v.size(); i++)
        REQUIRE(v[i] == static_cast<int>(i));
}

TEST_CASE("no_init_vector of a non-trivial type behaves like std::vector",
    "[Containers][no_init_vector]")
{
    no_init_vector<std::string> v;
    v.push_back("hello");
    v.push_back("world");
    v.resize(4);

    REQUIRE(v.size() == 4);
    REQUIRE(v[0] == "hello");
    REQUIRE(v[1] == "world");
    REQUIRE(v[2].empty());
    REQUIRE(v[3].empty());
}

TEST_CASE("no_init_vector allocation above the huge-page threshold round-trips values",
    "[Containers][no_init_vector]")
{
    // page_size is 2MiB (1<<21) and the mmap/posix_memalign path only kicks in at >= 2*page_size
    // bytes; use a count comfortably past that for int elements to exercise it.
    constexpr std::size_t count = (2 * (1UL << 21)) / sizeof(int) + 1024;
    no_init_vector<int> v;
    v.resize(count);
    for (std::size_t i = 0; i < count; i += 997)
        v[i] = static_cast<int>(i);

    for (std::size_t i = 0; i < count; i += 997)
        REQUIRE(v[i] == static_cast<int>(i));
}

TEST_CASE("no_init_vector compares equal to an equivalent std::vector in both directions",
    "[Containers][no_init_vector]")
{
    no_init_vector<int> niv;
    niv.push_back(1);
    niv.push_back(2);
    niv.push_back(3);

    std::vector<int> v { 1, 2, 3 };

    REQUIRE(niv == v);
    REQUIRE(v == niv);

    std::vector<int> different { 1, 2, 4 };
    REQUIRE_FALSE(niv == different);
    REQUIRE_FALSE(different == niv);
}
