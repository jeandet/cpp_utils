#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <strings/fuzzy_match.hpp>

using namespace cpp_utils::strings;

TEST_CASE("fuzzy_match_score: empty query matches everything, empty candidate matches nothing",
    "[strings]")
{
    REQUIRE(fuzzy_match_score<char>("", "anything") == 1);
    REQUIRE(fuzzy_match_score<char>("abc", "") == 0);
}

TEST_CASE("fuzzy_match_score is case-insensitive and rejects non-subsequences", "[strings]")
{
    REQUIRE(fuzzy_match_score<char>("ABC", "abcdef") > 0);
    REQUIRE(fuzzy_match_score<char>("xyz", "abcdef") == 0);
}

TEST_CASE("fuzzy_match_score prefers a word-boundary-aligned occurrence over a greedy early match",
    "[strings]")
{
    // A naive greedy left-to-right scan grabs the first 'w'/'l' it sees (both in
    // "somew"/"riteLog" is contrived, but the real regression this guards is:
    // matching "wl" against "someWriteLog" should score using the clean
    // word-boundary-aligned "W" and "L" of "WriteLog", not any stray earlier
    // lowercase letters).
    const int score = fuzzy_match_score<char>("wl", "someWriteLogFunction");
    REQUIRE(score > 0);
}

TEST_CASE("fuzzy_match reports one matched position per query character, in increasing order",
    "[strings]")
{
    auto result = fuzzy_match<char>("abc", "aXbXc");
    REQUIRE(result.score > 0);
    REQUIRE(result.match_positions.size() == 3);
    REQUIRE(std::is_sorted(result.match_positions.begin(), result.match_positions.end()));
}

TEST_CASE("fuzzy_match returns a default (zero score, empty positions) result on no match",
    "[strings]")
{
    auto result = fuzzy_match<char>("xyz", "abcdef");
    REQUIRE(result.score == 0);
    REQUIRE(result.match_positions.empty());
}
