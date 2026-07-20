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

// Requires the bshoshany-thread-pool dependency, pulled in transitively via
// parallel_for.hpp (see there for the -Dthreading=true rationale).
#include "../types/concepts.hpp"
#include "parallel_for.hpp"

#include <cstddef>
#include <ranges>
#include <span>
#include <utility>

namespace cpp_utils::threading
{

namespace details
{
    // Slices [start, start + count) out of input. Const-correctness of the resulting
    // span follows from Input's const-ness via std::ranges::data's return type.
    template <typename Input>
    auto make_span(Input& input, std::size_t start, std::size_t count)
    {
        return std::span(std::ranges::data(input) + start, count);
    }

    // Shared threshold/chunk-count policy for every parallel_chunks_* function: chunk_count
    // == 0 defaults to the pool's thread count; parallelize is false when min_chunk_size > 0
    // and count is below min_chunk_size * effective chunk count, in which case the caller
    // should make a single serial call instead of touching the pool.
    inline std::pair<bool, std::size_t> chunk_dispatch_plan(
        std::size_t count, std::size_t min_chunk_size, std::size_t chunk_count)
    {
        const std::size_t effective_chunks
            = chunk_count == 0 ? pool().get_thread_count() : chunk_count;
        const bool parallelize = !(min_chunk_size > 0 && count < min_chunk_size * effective_chunks);
        return { parallelize, effective_chunks };
    }
}

// Splits input into chunk_count contiguous chunks (0 = pool's thread count) and calls
// f(subspan) once per chunk, mutating chunks in place. Falls back to a single serial
// call f(whole input) when count < min_chunk_size * effective chunk count (pass
// min_chunk_size = 0 to always parallelize).
template <typename Input, typename F>
    requires types::concepts::chunk_for_each_callback<F, Input>
void parallel_chunks_for_each(
    Input& input, std::size_t min_chunk_size, F&& f, std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return;

    const auto [parallelize, effective_chunks]
        = details::chunk_dispatch_plan(count, min_chunk_size, chunk_count);
    if (!parallelize)
    {
        f(details::make_span(input, 0, count));
        return;
    }

    pool().detach_blocks(
        std::size_t { 0 }, count,
        [&](std::size_t start, std::size_t end)
        { f(details::make_span(input, start, end - start)); },
        effective_chunks);
    pool().wait();
}

// Same chunking as parallel_chunks_for_each, but pairs each read-only input chunk with the
// matching output offset: f(input_subspan, output + start). output must point to storage
// for at least std::ranges::size(input) elements — the caller's responsibility, not
// validated here (matches parallel_for's existing no-bounds-checking assumption).
template <typename Input, typename Output, typename F>
    requires types::concepts::pointer_to_contiguous_memory<Output>
          && types::concepts::chunk_transform_callback<F, Input, Output>
void parallel_chunks_transform(const Input& input, Output output, std::size_t min_chunk_size,
    F&& f, std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return;

    const auto [parallelize, effective_chunks]
        = details::chunk_dispatch_plan(count, min_chunk_size, chunk_count);
    if (!parallelize)
    {
        f(details::make_span(input, 0, count), output);
        return;
    }

    pool().detach_blocks(
        std::size_t { 0 }, count,
        [&](std::size_t start, std::size_t end)
        { f(details::make_span(input, start, end - start), output + start); },
        effective_chunks);
    pool().wait();
}

// Per-chunk f reduces a chunk to one T; combine folds the per-chunk partials together,
// starting from init. Below-threshold input skips the pool entirely:
// combine(init, f(whole input)). Empty input returns init untouched — f/combine are never
// called.
template <typename Input, typename T, typename F, typename BinaryOp>
    requires types::concepts::chunk_reduce_callback<F, Input, T>
          && types::concepts::foldable_binary_op<BinaryOp, T>
T parallel_chunks_reduce(const Input& input, T init, std::size_t min_chunk_size, F&& f,
    BinaryOp&& combine, std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return init;

    const auto [parallelize, effective_chunks]
        = details::chunk_dispatch_plan(count, min_chunk_size, chunk_count);
    if (!parallelize)
        return combine(init, f(details::make_span(input, 0, count)));

    auto partials = pool()
                        .submit_blocks(
                            std::size_t { 0 }, count,
                            [&](std::size_t start, std::size_t end)
                            { return f(details::make_span(input, start, end - start)); },
                            effective_chunks)
                        .get();

    T acc = init;
    for (auto& partial : partials)
        acc = combine(acc, partial);
    return acc;
}

}
