# Chunked Parallelism with Threshold (`threading::parallel_chunks_*`) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three chunk-based parallel primitives (`parallel_chunks_for_each`,
`parallel_chunks_transform`, `parallel_chunks_reduce`) to `cpp_utils::threading`, built on
`BS::thread_pool`'s existing block-splitting (`detach_blocks`/`submit_blocks`), each skipping
the pool entirely below a caller-supplied size threshold.

**Architecture:** New header `include/cpp_utils/threading/parallel_chunks.hpp` includes the
existing `parallel_for.hpp` to reuse its `pool()` singleton. Four new named concepts in
`include/cpp_utils/types/concepts.hpp` constrain the input range, the per-chunk callbacks, and
the reduce combiner. All three functions share one `details::make_span` slicing helper and one
threshold/chunk-count policy: `chunk_count == 0` defaults to the pool's thread count;
`min_chunk_size > 0` and `count < min_chunk_size * effective_chunks` triggers a single serial
call instead of touching the pool.

**Tech Stack:** C++20, Meson/Ninja, Catch2 v3, `BS::thread_pool` (`bshoshany-thread-pool` wrap,
`-Dthreading=true`).

**Spec:** `docs/superpowers/specs/2026-07-20-parallel-chunks-design.md`

## Global Constraints

- C++20 only — no C++23 constructs anywhere (`cpp_std=c++20` is hard-pinned in `meson.build`).
- New code lives behind `-Dthreading=true`, exactly like `parallel_for` — not part of
  `cpp_utils_dep`; a consumer must depend on `bshoshany-thread-pool` themselves.
- Built on `BS::thread_pool::detach_blocks`/`submit_blocks` for chunk splitting — no
  hand-rolled chunk-splitting math anywhere in this plan.
- `min_chunk_size` is an **element count**, not a byte size.
- Tests use plain Catch2 `TEST_CASE`/`REQUIRE` (matching `tests/threading/test_parallel_for.cpp`
  and `tests/types/concepts.cpp`), not the BDD `GIVEN`/`WHEN`/`THEN` style used in `serde` tests.
- No raw `(count, f)` chunked overload — every chunked function takes the input range directly
  (rejected in the spec as an unnecessary abstraction; `parallel_for` already covers index-only
  work with no backing data).
- CDFpp is not read or modified by this plan — cpp_utils only.
- REQUIRE with a multi-template-argument concept needs double parens
  (`REQUIRE((concept<A, B>));`) — a bare comma inside a macro argument breaks preprocessor
  parsing, since `REQUIRE` is itself a macro.
- The `count`/`effective_chunks`/threshold computation is byte-identical across all three
  functions — it lives in exactly one place, `details::chunk_dispatch_plan` (added in Task 2),
  and Tasks 3 and 4 call it rather than re-inlining it. Do not duplicate this block.

---

### Task 1: `contiguous_sized_range` + `chunk_for_each_callback` concepts

**Files:**
- Modify: `include/cpp_utils/types/concepts.hpp`
- Test: `tests/types/concepts.cpp`

**Interfaces:**
- Produces: `cpp_utils::types::concepts::contiguous_sized_range<T>` (bool concept — satisfied
  by any `std::ranges::contiguous_range` that is also `std::ranges::sized_range`, e.g.
  `std::vector`, `std::array`, `std::span`, `no_init_vector`); `chunk_for_each_callback<F,
  Input>` (bool concept — `F` invocable with `std::span<std::ranges::range_value_t<Input>>`,
  requires `contiguous_sized_range<Input>`).

This task needs no new build wiring and no `-Dthreading=true` — `types/concepts.hpp` and its
test have no thread-pool dependency.

- [ ] **Step 1: Write the failing test**

Open `tests/types/concepts.cpp`. Add `#include <array>`, `#include <list>`, `#include <span>`,
`#include <vector>` if not already present (`<vector>` already is), and append inside the
existing `TEST_CASE("Concepts", "[types]")` block (after the existing `random_access_buffer`
checks):

```cpp
    REQUIRE(contiguous_sized_range<std::vector<int>>);
    REQUIRE(contiguous_sized_range<std::array<int, 4>>);
    REQUIRE(!contiguous_sized_range<std::list<int>>);
    REQUIRE(!contiguous_sized_range<int>);

    auto good_for_each = [](std::span<int>) {};
    auto bad_arity = [](int) {};
    auto bad_element_type = [](std::span<double>) {};
    REQUIRE((chunk_for_each_callback<decltype(good_for_each), std::vector<int>>));
    REQUIRE(!(chunk_for_each_callback<decltype(bad_arity), std::vector<int>>));
    REQUIRE(!(chunk_for_each_callback<decltype(bad_element_type), std::vector<int>>));
```

- [ ] **Step 2: Run test to verify it fails**

The build directory is already configured with `-Dthreading=true` (required by later tasks in
this plan) — do not run `meson setup build` again, it will error with "Directory already
configured". Just build:

```bash
ninja -C build TestConcepts
```

Expected: FAIL — compile error, `use of undeclared identifier 'contiguous_sized_range'` (or
`'chunk_for_each_callback'`).

- [ ] **Step 3: Write minimal implementation**

In `include/cpp_utils/types/concepts.hpp`, add `<ranges>` and `<span>` to the includes (keep
`<concepts>`, `<type_traits>`, `<iterator>`):

```cpp
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
```

Then append, before the closing `} // namespace cpp_utils::types::concepts`:

```cpp
/** contiguous_sized_range: a range whose elements are contiguous in memory and whose size
 * is known in O(1) — std::vector, std::array, std::span, no_init_vector, etc. Shared
 * precondition for all threading::parallel_chunks_* input ranges. */
template <class T>
concept contiguous_sized_range = std::ranges::contiguous_range<T> && std::ranges::sized_range<T>;

/** chunk_for_each_callback: F is invocable with a mutable span over Input's element type —
 * the per-chunk callback shape used by threading::parallel_chunks_for_each. */
template <class F, class Input>
concept chunk_for_each_callback = contiguous_sized_range<Input> &&
    std::invocable<F, std::span<std::ranges::range_value_t<Input>>>;
```

- [ ] **Step 4: Run test to verify it passes**

```bash
ninja -C build TestConcepts
meson test -C build Test-TestConcepts
```

Expected: PASS, `1/1` tests pass, exit code 0.

- [ ] **Step 5: Commit**

```bash
git add include/cpp_utils/types/concepts.hpp tests/types/concepts.cpp \
    docs/superpowers/specs/2026-07-20-parallel-chunks-design.md \
    docs/superpowers/plans/2026-07-20-parallel-chunks.md
git commit -m "feat: add contiguous_sized_range and chunk_for_each_callback concepts"
```

---

### Task 2: `parallel_chunks_for_each`

**Files:**
- Create: `include/cpp_utils/threading/parallel_chunks.hpp`
- Create: `tests/threading/test_parallel_chunks.cpp`
- Modify: `meson.build` (root `headers` list)
- Modify: `tests/meson.build` (new `TestParallelChunks` target)

**Interfaces:**
- Consumes: `cpp_utils::threading::pool()` (`parallel_for.hpp`, returns `BS::light_thread_pool&`
  — has `.get_thread_count() -> std::size_t`, `.detach_blocks(first, last, block_fn,
  num_blocks=0)`, `.wait()`); `types::concepts::chunk_for_each_callback<F, Input>` (Task 1).
- Produces: `cpp_utils::threading::details::make_span(input, start, count) -> std::span<...>`;
  `cpp_utils::threading::details::chunk_dispatch_plan(count, min_chunk_size, chunk_count) ->
  std::pair<bool, std::size_t>` (first: whether to dispatch to the pool at all; second: the
  effective chunk count either way) — Tasks 3 and 4 reuse this, do not re-inline its logic;
  `cpp_utils::threading::parallel_chunks_for_each(Input& input, std::size_t min_chunk_size,
  F&& f, std::size_t chunk_count = 0)`.

- [ ] **Step 1: Wire the build system for the new files**

In `meson.build`, add a line to the `headers` list (alphabetically, right after the existing
`'include/cpp_utils/threading/parallel_for.hpp',` entry):

```
        'include/cpp_utils/threading/parallel_chunks.hpp',
```

In `tests/meson.build`, add a new entry to the `tests` list, right after the existing
`TestParallelFor` entry:

```
    {
        'name':'TestParallelChunks',
        'sources': ['threading/test_parallel_chunks.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, bshoshany_thread_pool_dep]
    },
```

- [ ] **Step 2: Write the failing test**

Create `tests/threading/test_parallel_chunks.cpp`:

```cpp
#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <threading/parallel_chunks.hpp>
#include <vector>

using namespace cpp_utils::threading;

TEST_CASE("parallel_chunks_for_each mutates every element exactly once above threshold", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<int> items(count, 0);
    parallel_chunks_for_each(items, std::size_t { 1 },
        [](std::span<int> chunk)
        {
            for (auto& v : chunk)
                v += 1;
        });
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each runs one serial call when below threshold", "[threading]")
{
    std::vector<int> items(4, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(items, std::size_t { 1'000'000 },
        [&](std::span<int> chunk)
        {
            calls++;
            REQUIRE(chunk.size() == items.size());
            for (auto& v : chunk)
                v += 1;
        });
    REQUIRE(calls == 1);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each respects an explicit chunk_count", "[threading]")
{
    constexpr std::size_t count = 100;
    std::vector<int> items(count, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(
        items, std::size_t { 1 },
        [&](std::span<int> chunk)
        {
            calls++;
            for (auto& v : chunk)
                v += 1;
        },
        std::size_t { 4 });
    REQUIRE(calls == 4);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each: min_chunk_size 0 disables the threshold gate", "[threading]")
{
    constexpr std::size_t count = 4;
    std::vector<int> items(count, 0);
    std::atomic<int> calls { 0 };
    parallel_chunks_for_each(
        items, std::size_t { 0 },
        [&](std::span<int> chunk)
        {
            calls++;
            for (auto& v : chunk)
                v += 1;
        },
        std::size_t { 4 });
    REQUIRE(calls == 4);
    for (auto v : items)
        REQUIRE(v == 1);
}

TEST_CASE("parallel_chunks_for_each is a no-op for empty input", "[threading]")
{
    std::vector<int> items;
    int calls = 0;
    parallel_chunks_for_each(items, std::size_t { 1 }, [&](std::span<int>) { calls++; });
    REQUIRE(calls == 0);
}
```

- [ ] **Step 3: Run test to verify it fails**

```bash
ninja -C build
```

Expected: FAIL — `include/cpp_utils/threading/parallel_chunks.hpp` doesn't exist yet
(`fatal error: 'threading/parallel_chunks.hpp' file not found`).

- [ ] **Step 4: Write minimal implementation**

Create `include/cpp_utils/threading/parallel_chunks.hpp`:

```cpp
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

}
```

- [ ] **Step 5: Run test to verify it passes**

```bash
ninja -C build TestParallelChunks
meson test -C build Test-TestParallelChunks
```

Expected: PASS, all 5 `TEST_CASE`s pass, exit code 0.

- [ ] **Step 6: Commit**

```bash
git add include/cpp_utils/threading/parallel_chunks.hpp tests/threading/test_parallel_chunks.cpp \
    meson.build tests/meson.build
git commit -m "feat: add threading::parallel_chunks_for_each"
```

---

### Task 3: `parallel_chunks_transform`

**Files:**
- Modify: `include/cpp_utils/types/concepts.hpp`
- Modify: `include/cpp_utils/threading/parallel_chunks.hpp`
- Modify: `tests/types/concepts.cpp`
- Modify: `tests/threading/test_parallel_chunks.cpp`

**Interfaces:**
- Consumes: `details::make_span` and `details::chunk_dispatch_plan(count, min_chunk_size,
  chunk_count) -> std::pair<bool, std::size_t>` from Task 2 — call it, do not re-inline its
  logic; `types::concepts::pointer_to_contiguous_memory<Output>` (existing, `types/concepts.hpp`).
- Produces: `types::concepts::chunk_transform_callback<F, Input, Output>`;
  `cpp_utils::threading::parallel_chunks_transform(const Input& input, Output output,
  std::size_t min_chunk_size, F&& f, std::size_t chunk_count = 0)`.

- [ ] **Step 1: Write the failing test (concept)**

Append inside `TEST_CASE("Concepts", "[types]")` in `tests/types/concepts.cpp`:

```cpp
    auto good_transform = [](std::span<const int>, int*) {};
    auto bad_output_type = [](std::span<const int>, double*) {};
    REQUIRE((chunk_transform_callback<decltype(good_transform), std::vector<int>, int*>));
    REQUIRE(!(chunk_transform_callback<decltype(bad_output_type), std::vector<int>, int*>));
```

- [ ] **Step 2: Run test to verify it fails**

```bash
ninja -C build TestConcepts
```

Expected: FAIL — `use of undeclared identifier 'chunk_transform_callback'`.

- [ ] **Step 3: Write minimal implementation (concept)**

Append to `include/cpp_utils/types/concepts.hpp`, after `chunk_for_each_callback`:

```cpp
/** chunk_transform_callback: F is invocable with a read-only input chunk and an output
 * position — the per-chunk callback shape used by threading::parallel_chunks_transform. */
template <class F, class Input, class Output>
concept chunk_transform_callback = contiguous_sized_range<Input> &&
    std::invocable<F, std::span<const std::ranges::range_value_t<Input>>, Output>;
```

- [ ] **Step 4: Run test to verify it passes**

```bash
ninja -C build TestConcepts
meson test -C build Test-TestConcepts
```

Expected: PASS.

- [ ] **Step 5: Write the failing test (function)**

Append to `tests/threading/test_parallel_chunks.cpp` (add `#include <numeric>` near the top for
`std::iota`):

```cpp
TEST_CASE("parallel_chunks_transform writes correct output offsets above threshold", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<int> input(count);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(count, -1);

    parallel_chunks_transform(input, output.data(), std::size_t { 1 },
        [](std::span<const int> chunk, int* out)
        {
            for (std::size_t i = 0; i < chunk.size(); ++i)
                out[i] = chunk[i] * 2;
        });

    for (std::size_t i = 0; i < count; ++i)
        REQUIRE(output[i] == static_cast<int>(i) * 2);
}

TEST_CASE("parallel_chunks_transform runs one serial call when below threshold", "[threading]")
{
    std::vector<int> input { 1, 2, 3, 4 };
    std::vector<int> output(4, -1);
    std::atomic<int> calls { 0 };

    parallel_chunks_transform(input, output.data(), std::size_t { 1'000'000 },
        [&](std::span<const int> chunk, int* out)
        {
            calls++;
            REQUIRE(chunk.size() == input.size());
            for (std::size_t i = 0; i < chunk.size(); ++i)
                out[i] = chunk[i];
        });

    REQUIRE(calls == 1);
    REQUIRE(output == input);
}
```

- [ ] **Step 6: Run test to verify it fails**

```bash
ninja -C build TestParallelChunks
```

Expected: FAIL — `use of undeclared identifier 'parallel_chunks_transform'`.

- [ ] **Step 7: Write minimal implementation (function)**

Append to `include/cpp_utils/threading/parallel_chunks.hpp`, after `parallel_chunks_for_each`:

```cpp
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
```

- [ ] **Step 8: Run test to verify it passes**

```bash
ninja -C build TestParallelChunks
meson test -C build Test-TestParallelChunks
```

Expected: PASS, `7/7` `TEST_CASE`s pass.

- [ ] **Step 9: Commit**

```bash
git add include/cpp_utils/types/concepts.hpp include/cpp_utils/threading/parallel_chunks.hpp \
    tests/types/concepts.cpp tests/threading/test_parallel_chunks.cpp
git commit -m "feat: add threading::parallel_chunks_transform"
```

---

### Task 4: `parallel_chunks_reduce`

**Files:**
- Modify: `include/cpp_utils/types/concepts.hpp`
- Modify: `include/cpp_utils/threading/parallel_chunks.hpp`
- Modify: `tests/types/concepts.cpp`
- Modify: `tests/threading/test_parallel_chunks.cpp`

**Interfaces:**
- Consumes: `pool().submit_blocks(first, last, block_fn, num_blocks) -> BS::multi_future<R>`
  where `R` is `block_fn`'s return type (`BS_thread_pool.hpp:1835`);
  `BS::multi_future<R>::get() -> std::vector<R>` (`BS_thread_pool.hpp:470`); `details::make_span`
  and `details::chunk_dispatch_plan(count, min_chunk_size, chunk_count) -> std::pair<bool,
  std::size_t>` from Task 2 — call it, do not re-inline its logic.
- Produces: `types::concepts::chunk_reduce_callback<F, Input, T>`;
  `types::concepts::foldable_binary_op<BinaryOp, T>`; `cpp_utils::threading::parallel_chunks_reduce(
  const Input& input, T init, std::size_t min_chunk_size, F&& f, BinaryOp&& combine,
  std::size_t chunk_count = 0) -> T`.

- [ ] **Step 1: Write the failing test (concepts)**

Append inside `TEST_CASE("Concepts", "[types]")` in `tests/types/concepts.cpp` (add
`#include <numeric>` and `#include <string>` near the top for `std::accumulate`/`std::to_string`):

```cpp
    auto sum_chunk
        = [](std::span<const int> chunk) { return std::accumulate(chunk.begin(), chunk.end(), 0); };
    auto bad_reduce = [](std::span<const int>) { return std::string {}; };
    REQUIRE((chunk_reduce_callback<decltype(sum_chunk), std::vector<int>, int>));
    REQUIRE(!(chunk_reduce_callback<decltype(bad_reduce), std::vector<int>, int>));

    auto plus_op = [](int a, int b) { return a + b; };
    auto bad_op = [](int a, int b) { return std::to_string(a + b); };
    REQUIRE((foldable_binary_op<decltype(plus_op), int>));
    REQUIRE(!(foldable_binary_op<decltype(bad_op), int>));
```

- [ ] **Step 2: Run test to verify it fails**

```bash
ninja -C build TestConcepts
```

Expected: FAIL — `use of undeclared identifier 'chunk_reduce_callback'`.

- [ ] **Step 3: Write minimal implementation (concepts)**

Append to `include/cpp_utils/types/concepts.hpp`, after `chunk_transform_callback`:

```cpp
/** chunk_reduce_callback: F reduces a read-only input chunk down to a single value
 * convertible to T — the per-chunk callback shape used by threading::parallel_chunks_reduce. */
template <class F, class Input, class T>
concept chunk_reduce_callback = contiguous_sized_range<Input> &&
    std::regular_invocable<F, std::span<const std::ranges::range_value_t<Input>>> &&
    std::convertible_to<std::invoke_result_t<F, std::span<const std::ranges::range_value_t<Input>>>, T>;

/** foldable_binary_op: BinaryOp(T, T) -> convertible-to-T, e.g. std::plus<>{} — the combine
 * step used by threading::parallel_chunks_reduce to fold per-chunk partial results. */
template <class BinaryOp, class T>
concept foldable_binary_op = std::regular_invocable<BinaryOp, T, T> &&
    std::convertible_to<std::invoke_result_t<BinaryOp, T, T>, T>;
```

- [ ] **Step 4: Run test to verify it passes**

```bash
ninja -C build TestConcepts
meson test -C build Test-TestConcepts
```

Expected: PASS.

- [ ] **Step 5: Write the failing test (function)**

Append to `tests/threading/test_parallel_chunks.cpp`:

```cpp
TEST_CASE("parallel_chunks_reduce matches a serial accumulate above threshold", "[threading]")
{
    constexpr std::size_t count = 1000;
    std::vector<int> input(count);
    std::iota(input.begin(), input.end(), 1);

    auto result = parallel_chunks_reduce(
        input, 0, std::size_t { 1 },
        [](std::span<const int> chunk) { return std::accumulate(chunk.begin(), chunk.end(), 0); },
        [](int a, int b) { return a + b; });

    REQUIRE(result == std::accumulate(input.begin(), input.end(), 0));
}

TEST_CASE("parallel_chunks_reduce runs one serial call when below threshold", "[threading]")
{
    std::vector<int> input { 1, 2, 3, 4 };
    std::atomic<int> calls { 0 };

    auto result = parallel_chunks_reduce(
        input, 10, std::size_t { 1'000'000 },
        [&](std::span<const int> chunk)
        {
            calls++;
            return std::accumulate(chunk.begin(), chunk.end(), 0);
        },
        [](int a, int b) { return a + b; });

    REQUIRE(calls == 1);
    REQUIRE(result == 20);
}

TEST_CASE("parallel_chunks_reduce returns init unchanged for empty input", "[threading]")
{
    std::vector<int> input;
    int f_calls = 0;
    int combine_calls = 0;

    auto result = parallel_chunks_reduce(
        input, 42, std::size_t { 1 },
        [&](std::span<const int>)
        {
            f_calls++;
            return 0;
        },
        [&](int a, int b)
        {
            combine_calls++;
            return a + b;
        });

    REQUIRE(result == 42);
    REQUIRE(f_calls == 0);
    REQUIRE(combine_calls == 0);
}
```

(`std::accumulate` needs `#include <numeric>`, already added in Task 3's Step 5.)

- [ ] **Step 6: Run test to verify it fails**

```bash
ninja -C build TestParallelChunks
```

Expected: FAIL — `use of undeclared identifier 'parallel_chunks_reduce'`.

- [ ] **Step 7: Write minimal implementation (function)**

Append to `include/cpp_utils/threading/parallel_chunks.hpp`, after `parallel_chunks_transform`:

```cpp
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
```

- [ ] **Step 8: Run test to verify it passes**

```bash
ninja -C build TestParallelChunks
meson test -C build Test-TestParallelChunks
```

Expected: PASS, `10/10` `TEST_CASE`s pass.

- [ ] **Step 9: Commit**

```bash
git add include/cpp_utils/types/concepts.hpp include/cpp_utils/threading/parallel_chunks.hpp \
    tests/types/concepts.cpp tests/threading/test_parallel_chunks.cpp
git commit -m "feat: add threading::parallel_chunks_reduce"
```

---

### Task 5: Document the new module and do a final full-suite verification

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: nothing new — this task only documents and verifies Tasks 1-4's output.

- [ ] **Step 1: Update CLAUDE.md's threading/ bullet**

In `CLAUDE.md`, find the `threading/` bullet in the module-layout list:

```
- `threading/` — `parallel_for`/`parallel_for_each` over a lazily-initialized, hardware-
  concurrency-sized `BS::light_thread_pool` singleton. Needs the `bshoshany-thread-pool`
  dependency explicitly (see Build & test above) — not part of `cpp_utils_dep`.
```

Replace it with:

```
- `threading/` — `parallel_for`/`parallel_for_each` over a lazily-initialized, hardware-
  concurrency-sized `BS::light_thread_pool` singleton. Needs the `bshoshany-thread-pool`
  dependency explicitly (see Build & test above) — not part of `cpp_utils_dep`. Also
  `parallel_chunks_for_each`/`parallel_chunks_transform`/`parallel_chunks_reduce`
  (`parallel_chunks.hpp`) — chunk-based dispatch built on `BS::thread_pool`'s own
  `detach_blocks`/`submit_blocks` block-splitting rather than hand-rolled chunk math, each
  skipping the pool for a single serial call when the input is smaller than a caller-supplied
  `min_chunk_size * chunk_count` threshold. Distinct from `parallel_for`: chunked functions
  hand each chunk a contiguous `std::span` of real data, not a bare index.
```

- [ ] **Step 2: Full verification from scratch**

```bash
rm -rf build
meson setup build -Dthreading=true
ninja -C build
meson test -C build
```

Expected: all tests pass (28 targets — the existing 27 plus the new `TestParallelChunks`; read
the actual printed pass count and exit code, don't assume). Also verify the default
(`-Dthreading` unset) configuration still builds clean, since a prior incident in this project
was a threading dependency accidentally forced into the default build:

```bash
rm -rf build
meson setup build
ninja -C build
meson test -C build
```

Expected: same pass count as before this plan (no `TestParallelChunks` target present, no
network fetch attempted).

- [ ] **Step 3: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: document threading::parallel_chunks_* in CLAUDE.md"
```

## Self-Review Notes

- **Spec coverage:** all 7 numbered Design sections in the spec map to a task —
  §1 concepts → Tasks 1/3/4; §2 `make_span` → Task 2; §3 `parallel_chunks_for_each` → Task 2;
  §4 `parallel_chunks_transform` → Task 3; §5 `parallel_chunks_reduce` → Task 4; §6
  threshold/chunk-count semantics → tested in every task (below-threshold, `chunk_count`
  override, `min_chunk_size == 0`, `count == 0`); §7 file placement → Task 2 Step 1. The
  spec's Testing section's bullets are covered 1:1 by the `TEST_CASE`s in Tasks 2-4.
- **Placeholder scan:** none — every step has literal file contents and exact commands.
- **Type consistency:** `parallel_chunks_for_each(Input&, std::size_t, F&&, std::size_t=0)`,
  `parallel_chunks_transform(const Input&, Output, std::size_t, F&&, std::size_t=0)`,
  `parallel_chunks_reduce(const Input&, T, std::size_t, F&&, BinaryOp&&, std::size_t=0) -> T`
  are used identically in every task that references them; `details::make_span` signature is
  identical across Tasks 2-4.
