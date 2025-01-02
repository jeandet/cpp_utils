#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils/types/concepts.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>


constexpr bool is_container(auto&& )
{
    return false;
}

constexpr bool is_container(cpp_utils::types::concepts::container auto&& )
{
    return true;
}

constexpr bool is_sequence_container(auto&& )
{
    return false;
}

constexpr bool is_sequence_container(cpp_utils::types::concepts::sequence_container auto&& )
{
    return true;
}

constexpr bool is_contiguous_sequence_container(auto&& )
{
    return false;
}

constexpr bool is_contiguous_sequence_container(cpp_utils::types::concepts::contiguous_sequence_container auto&& )
{
    return true;
}


TEST_CASE("Containers concepts", "[Container]")
{
    REQUIRE(is_container(std::list<int>{}));
    REQUIRE(is_container(std::vector<int>{}));
    REQUIRE(is_container(std::deque<int>{}));
    // forward_list does not satisfy the Container v.size() requirement
    REQUIRE(!is_container(std::forward_list<int>{}));
    REQUIRE(is_container(std::array<int,3>{}));
    REQUIRE(is_container(std::set<int>{}));
    REQUIRE(is_container(std::map<int, int>{}));
}

TEST_CASE("Containers concepts", "[Container Sequence]")
{
    REQUIRE(is_sequence_container(std::list<int>{}));
    REQUIRE(is_sequence_container(std::vector<int>{}));
    REQUIRE(is_sequence_container(std::string{}));
}

TEST_CASE("Containers concepts", "[Container Contiguous Sequence]")
{
    REQUIRE(is_contiguous_sequence_container(std::vector<int>{}));
    REQUIRE(is_contiguous_sequence_container(std::array<int,3>{}));
    // forward_list does not satisfy the Contiguous Sequence v.size() requirement
    REQUIRE(!is_contiguous_sequence_container(std::forward_list<int>{}));
}
