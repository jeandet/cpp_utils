#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2026, Plasma Physics Laboratory - CNRS
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
--                            alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cwctype>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cpp_utils::strings
{

struct fuzzy_match_result
{
    int score = 0;
    std::vector<int> match_positions;
};

namespace details
{
    template <typename CharT>
    constexpr CharT to_lower(CharT c) noexcept
    {
        if constexpr (std::is_same_v<CharT, char>)
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else
            return static_cast<CharT>(std::towlower(static_cast<std::wint_t>(c)));
    }

    template <typename CharT>
    constexpr bool is_upper(CharT c) noexcept
    {
        if constexpr (std::is_same_v<CharT, char>)
            return std::isupper(static_cast<unsigned char>(c)) != 0;
        else
            return std::iswupper(static_cast<std::wint_t>(c)) != 0;
    }

    template <typename CharT>
    constexpr bool is_lower(CharT c) noexcept
    {
        if constexpr (std::is_same_v<CharT, char>)
            return std::islower(static_cast<unsigned char>(c)) != 0;
        else
            return std::iswlower(static_cast<std::wint_t>(c)) != 0;
    }

    // Bonus for matching at candidate position i: start of string, right after a
    // separator ('_', ' ', '/'), or a lower->Upper camelCase transition. Depends
    // only on the position, not on which earlier position was matched before it.
    template <typename CharT>
    int word_start_bonus(std::basic_string_view<CharT> candidate, std::size_t i)
    {
        if (i == 0)
            return 3;
        const CharT prev = candidate[i - 1];
        if (prev == CharT('_') || prev == CharT(' ') || prev == CharT('/'))
            return 3;
        if (is_upper(candidate[i]) && is_lower(prev))
            return 2;
        return 0;
    }

    inline constexpr int neg_inf = std::numeric_limits<int>::min() / 2;

    // Highest-scoring way to match `query` as a subsequence of `candidate`.
    //
    // A naive left-to-right greedy scan commits to the first occurrence of each
    // query character wherever it happens to appear, which can "waste" an early
    // throwaway match and then miss a clean, word-boundary-aligned occurrence of
    // the whole query later in the same text. This DP instead considers every
    // valid alignment and keeps the best-scoring one.
    //
    // O(candidate.size() * query.size()) time, O(candidate.size()) space - no
    // backpointers, since only the score is needed here (see
    // best_alignment_with_positions for the position-reporting variant).
    template <typename CharT>
    int best_alignment_raw_score(
        std::basic_string_view<CharT> query, std::basic_string_view<CharT> candidate)
    {
        const std::size_t n = candidate.size();
        const std::size_t m = query.size();
        if (m > n)
            return -1;

        std::vector<int> prev_level(n, neg_inf);
        std::vector<int> cur_level(n, neg_inf);

        for (std::size_t i = 0; i < n; ++i)
            if (to_lower(query[0]) == to_lower(candidate[i]))
                prev_level[i] = 1 + word_start_bonus(candidate, i);

        for (std::size_t j = 1; j < m; ++j)
        {
            std::fill(cur_level.begin(), cur_level.end(), neg_inf);
            int prefix_max = neg_inf;

            for (std::size_t i = 0; i < n; ++i)
            {
                const int adjacent
                    = (i >= 1 && prev_level[i - 1] != neg_inf) ? prev_level[i - 1] + 4 : neg_inf;
                const int best_prev = std::max(prefix_max, adjacent);

                if (best_prev != neg_inf && to_lower(query[j]) == to_lower(candidate[i]))
                    cur_level[i] = best_prev + 1 + word_start_bonus(candidate, i);

                if (i >= 1 && prev_level[i - 1] > prefix_max)
                    prefix_max = prev_level[i - 1];
            }
            prev_level.swap(cur_level);
        }

        int best = neg_inf;
        for (std::size_t i = 0; i < n; ++i)
            best = std::max(best, prev_level[i]);
        return best == neg_inf ? -1 : best;
    }

    // Same recurrence as best_alignment_raw_score, but keeps a full backpointer
    // table to reconstruct the winning positions.
    template <typename CharT>
    int best_alignment_with_positions(std::basic_string_view<CharT> query,
        std::basic_string_view<CharT> candidate, std::vector<int>& out_positions)
    {
        const std::size_t n = candidate.size();
        const std::size_t m = query.size();
        if (m > n)
            return -1;

        std::vector<std::vector<int>> level_score(m, std::vector<int>(n, neg_inf));
        std::vector<std::vector<long>> level_back(m, std::vector<long>(n, -1));

        for (std::size_t i = 0; i < n; ++i)
            if (to_lower(query[0]) == to_lower(candidate[i]))
                level_score[0][i] = 1 + word_start_bonus(candidate, i);

        for (std::size_t j = 1; j < m; ++j)
        {
            const auto& prev_score = level_score[j - 1];
            int prefix_max = neg_inf;
            long prefix_max_pos = -1;

            for (std::size_t i = 0; i < n; ++i)
            {
                const int adjacent
                    = (i >= 1 && prev_score[i - 1] != neg_inf) ? prev_score[i - 1] + 4 : neg_inf;
                int best_prev = prefix_max;
                long best_prev_pos = prefix_max_pos;
                if (adjacent >= best_prev)
                {
                    best_prev = adjacent;
                    best_prev_pos = static_cast<long>(i) - 1;
                }

                if (best_prev != neg_inf && to_lower(query[j]) == to_lower(candidate[i]))
                {
                    level_score[j][i] = best_prev + 1 + word_start_bonus(candidate, i);
                    level_back[j][i] = best_prev_pos;
                }

                if (i >= 1 && prev_score[i - 1] > prefix_max)
                {
                    prefix_max = prev_score[i - 1];
                    prefix_max_pos = static_cast<long>(i) - 1;
                }
            }
        }

        int best = neg_inf;
        long best_pos = -1;
        const auto& last_level = level_score[m - 1];
        for (std::size_t i = 0; i < n; ++i)
        {
            if (last_level[i] > best)
            {
                best = last_level[i];
                best_pos = static_cast<long>(i);
            }
        }
        if (best_pos < 0)
            return -1;

        out_positions.resize(m);
        long pos = best_pos;
        for (long j = static_cast<long>(m) - 1; j >= 0; --j)
        {
            out_positions[j] = static_cast<int>(pos);
            pos = level_back[j][pos];
        }
        return best;
    }
}

// Fuzzy subsequence-match score of `query` inside `candidate` (order-preserving,
// not necessarily contiguous), biased towards word-boundary/camelCase-aligned
// matches and penalized by leftover unmatched candidate length. 0 means no
// match; an empty query always scores 1.
//
// CharT must support std::tolower/isupper/islower semantics (char) or their
// wide-character equivalents (wchar_t) - only ASCII-range case-folding is
// correct, this is not a full Unicode case-fold.
template <typename CharT>
int fuzzy_match_score(
    std::basic_string_view<CharT> query, std::basic_string_view<CharT> candidate)
{
    if (query.empty())
        return 1;
    if (candidate.empty())
        return 0;

    const int raw = details::best_alignment_raw_score(query, candidate);
    if (raw < 0)
        return 0;

    const int length_penalty = static_cast<int>(candidate.size() - query.size());
    return std::max(1, raw * 100 / (100 + length_penalty));
}

// Same scoring as fuzzy_match_score(), also reporting the matched candidate
// positions (one per query character, in query order).
template <typename CharT>
fuzzy_match_result fuzzy_match(
    std::basic_string_view<CharT> query, std::basic_string_view<CharT> candidate)
{
    fuzzy_match_result result;
    if (query.empty())
    {
        result.score = 1;
        return result;
    }
    if (candidate.empty())
        return result;

    std::vector<int> positions;
    const int raw = details::best_alignment_with_positions(query, candidate, positions);
    if (raw < 0)
        return result;

    const int length_penalty = static_cast<int>(candidate.size() - query.size());
    result.score = std::max(1, raw * 100 / (100 + length_penalty));
    result.match_positions = std::move(positions);
    return result;
}

}
