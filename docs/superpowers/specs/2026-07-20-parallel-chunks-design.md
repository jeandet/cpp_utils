# Chunked parallelism with threshold (`threading::parallel_chunks_*`)

Date: 2026-07-20
Status: approved (design), pending implementation plan

## Problem

The generic-utility audit (`2026-07-17`) flagged CDFpp's `chrono::_impl::_thread_if_needed`
(`include/cdfpp/chrono/cdf-chrono.hpp:56-104`) as a candidate for reconciliation with
`cpp_utils::threading::parallel_for`, but deferred it: `_thread_if_needed` has exactly one
call site, so its API shape was unproven, and building a generic primitive against a single
use case would be a premature abstraction.

`_thread_if_needed` is a **chunked transform**, not an index-based fan-out like
`parallel_for`: it splits an input span into `threads_count` contiguous chunks, hands each
chunk (plus a matching output-pointer offset) to a worker via a raw `std::thread` spawned
per call, and — the part `parallel_for` has no equivalent of — skips parallelism entirely
below a size threshold (`count < min_chunk_size * threads_count`), because for
memory-bandwidth-bound work, threading below that size doesn't pay for itself.

The user has now judged that chunked-parallelism-with-a-size-threshold is common enough
(not just CDFpp's problem) to justify a general primitive in `cpp_utils` directly, without
waiting for a second real call site. This spec covers that primitive.

## Scope

Building three functions in `cpp_utils::threading`, plus their supporting concepts in
`cpp_utils::types::concepts`, validated by cpp_utils's own tests. **CDFpp is not touched** —
neither read-modified nor migrated to use the new primitive. This spec covers `cpp_utils`
only.

### Non-goals

- Migrating CDFpp's `_thread_if_needed` call site to the new primitive (future work, once
  cpp_utils's own migration onto the unified engine — see
  `2026-07-15-unified-reflexion-serde-engine-design.md` — is further along; not blocking,
  just not in scope here).
- A raw `(count, f)` chunked primitive with no backing container. Rejected during design:
  `parallel_for` already covers index-only work with no real data behind it: a chunked
  variant's entire point is handing each chunk a real slice of data, so it takes the input
  range directly (mirroring `parallel_for_each(container&, f)`), not an abstract count.
- Reconciling CDFpp's `ideal_threads_count()` heuristic (8 threads if
  `hardware_concurrency() >= 32` else 2) into `cpp_utils`. The new primitive's chunk count
  defaults to the pool's thread count and is otherwise caller-supplied — no bandwidth-based
  heuristic is baked in, since that heuristic is workload-dependent, not a library-level
  default.
- Hand-rolled chunk-splitting math. `BS::thread_pool::detach_blocks`/`submit_blocks`
  (`subprojects/thread-pool-5.1.0/include/BS_thread_pool.hpp:1531,1835`) already split a
  range into blocks with correct remainder distribution; reimplementing that would just be
  duplicated, untested logic.

## Constraints

- **C++20 only.** `cpp_utils` hard-pins `cpp_std=c++20`; no C++23 constructs.
- **Gated behind `-Dthreading=true`**, same pattern as `parallel_for` — depends on
  `bshoshany-thread-pool`, deliberately kept out of `cpp_utils_dep` (see `CLAUDE.md`).
- **Built on `BS::thread_pool`'s own block-splitting**, not hand-rolled chunk math (see
  Non-goals). `detach_blocks`/`submit_blocks` default `num_blocks = 0` to "the pool's thread
  count" and internally clamp `num_blocks` to the input size when it's smaller
  (`BS_thread_pool.hpp:589`) — the new primitive relies on both of these rather than
  reimplementing them.

## Design

### 1. Concepts (`types/concepts.hpp`)

Reuses existing standard concepts and the existing `pointer_to_contiguous_memory`; adds four
new named concepts (no inline ad-hoc `requires` composition at the call site):

```cpp
template <class T>
concept contiguous_sized_range = std::ranges::contiguous_range<T> && std::ranges::sized_range<T>;

template <class F, class Input>
concept chunk_for_each_callback = contiguous_sized_range<Input> &&
    std::invocable<F, std::span<std::ranges::range_value_t<Input>>>;

template <class F, class Input, class Output>
concept chunk_transform_callback = contiguous_sized_range<Input> &&
    std::invocable<F, std::span<const std::ranges::range_value_t<Input>>, Output>;

template <class F, class Input, class T>
concept chunk_reduce_callback = contiguous_sized_range<Input> &&
    std::regular_invocable<F, std::span<const std::ranges::range_value_t<Input>>> &&
    std::convertible_to<std::invoke_result_t<F, std::span<const std::ranges::range_value_t<Input>>>, T>;

template <class BinaryOp, class T>
concept foldable_binary_op = std::regular_invocable<BinaryOp, T, T> &&
    std::convertible_to<std::invoke_result_t<BinaryOp, T, T>, T>;
```

`contiguous_sized_range` is satisfied by `std::vector`, `std::array`, `std::span`,
`no_init_vector`, etc. — anything already usable with `parallel_for_each`.

### 2. Shared slicing helper

A single `details::make_span` builds the per-chunk `std::span`; const-correctness falls out
automatically from `Input`'s const-ness via `std::ranges::data`'s return type (const `Input&`
→ pointer-to-const → `std::span<const T>`; non-const `Input&` → mutable span) — no separate
const/non-const overloads needed:

```cpp
namespace details
{
    template <typename Input>
    auto make_span(Input& input, std::size_t start, std::size_t count)
    {
        return std::span(std::ranges::data(input) + start, count);
    }
}
```

### 3. `parallel_chunks_for_each`

Chunk-wise sibling of `parallel_for_each`. `input` is non-const so `f` can mutate chunks in
place (matches `parallel_for_each`'s existing mutability model in the same module).

```cpp
template <typename Input, typename F>
    requires types::concepts::chunk_for_each_callback<F, Input>
void parallel_chunks_for_each(Input& input, std::size_t min_chunk_size, F&& f,
                               std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return;

    const std::size_t effective_chunks = chunk_count == 0 ? pool().get_thread_count() : chunk_count;
    if (min_chunk_size > 0 && count < min_chunk_size * effective_chunks)
    {
        f(details::make_span(input, 0, count));
        return;
    }

    pool().detach_blocks(std::size_t { 0 }, count,
        [&](std::size_t start, std::size_t end)
        { f(details::make_span(input, start, end - start)); },
        effective_chunks);
    pool().wait();
}
```

### 4. `parallel_chunks_transform`

Matches CDFpp's `_thread_if_needed(input, output, f)` shape: read-only input, separate
output. `output` is constrained by the existing `pointer_to_contiguous_memory` concept
(covers raw pointers and anything with the same arithmetic/dereference shape).

```cpp
template <typename Input, typename Output, typename F>
    requires types::concepts::pointer_to_contiguous_memory<Output>
          && types::concepts::chunk_transform_callback<F, Input, Output>
void parallel_chunks_transform(const Input& input, Output output, std::size_t min_chunk_size,
                                F&& f, std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return;

    const std::size_t effective_chunks = chunk_count == 0 ? pool().get_thread_count() : chunk_count;
    if (min_chunk_size > 0 && count < min_chunk_size * effective_chunks)
    {
        f(details::make_span(input, 0, count), output);
        return;
    }

    pool().detach_blocks(std::size_t { 0 }, count,
        [&](std::size_t start, std::size_t end)
        { f(details::make_span(input, start, end - start), output + start); },
        effective_chunks);
    pool().wait();
}
```

`output` must point to storage for at least `count` elements — same contract as CDFpp's
`_thread_if_needed`, not re-validated here (matches the rest of the library's existing
assumption that the caller has already sized buffers correctly).

### 5. `parallel_chunks_reduce`

Per-chunk `f` reduces a chunk to one `T`; `combine` folds the per-chunk partials together,
starting from `init`. Uses `submit_blocks` (not `detach_blocks`) to collect one return value
per chunk via `BS::multi_future<T>::get()`, which returns `std::vector<T>`
(`BS_thread_pool.hpp:470`).

```cpp
template <typename Input, typename T, typename F, typename BinaryOp>
    requires types::concepts::chunk_reduce_callback<F, Input, T>
          && types::concepts::foldable_binary_op<BinaryOp, T>
T parallel_chunks_reduce(const Input& input, T init, std::size_t min_chunk_size, F&& f,
                          BinaryOp&& combine, std::size_t chunk_count = 0)
{
    const std::size_t count = std::ranges::size(input);
    if (count == 0)
        return init;

    const std::size_t effective_chunks = chunk_count == 0 ? pool().get_thread_count() : chunk_count;
    if (min_chunk_size > 0 && count < min_chunk_size * effective_chunks)
        return combine(init, f(details::make_span(input, 0, count)));

    auto partials = pool()
        .submit_blocks(std::size_t { 0 }, count,
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

### 6. Threshold & chunk-count semantics (all three functions)

- `chunk_count == 0` → defaults to `pool().get_thread_count()`.
- `min_chunk_size == 0` → threshold check `count < 0 * effective_chunks` is always false, so
  it always parallelizes (matches plain `parallel_for`'s no-threshold behavior).
- `min_chunk_size > 0` and `count < min_chunk_size * effective_chunks` → single serial call
  over the whole input, no pool involvement.
- `chunk_count` larger than `count` is handled inside `BS::thread_pool`'s own `blocks` helper
  (clamps to `count`), not re-validated here.
- `count == 0` → no-op for `for_each`/`transform`; `reduce` returns `init` untouched, neither
  `f` nor `combine` is ever called.

### 7. File placement

New header `include/cpp_utils/threading/parallel_chunks.hpp` (separate file from
`parallel_for.hpp`, same `threading/` directory) — keeps each file focused. It `#include`s
`parallel_for.hpp` to reuse the existing `pool()` singleton rather than redefining it — one
process-wide thread pool for the whole `threading` module, not one per header. Concepts go
into the existing `include/cpp_utils/types/concepts.hpp`. Both new/changed headers registered
in root `meson.build`'s `headers` list.

New test `tests/threading/test_parallel_chunks.cpp`, registered as `Test-TestParallelChunks`
in `tests/meson.build` with the same deps as `TestParallelFor`
(`[cpp_utils_dep, catch2_dep, bshoshany_thread_pool_dep]`), under the existing
`-Dthreading=true` gate.

## Testing

Plain `TEST_CASE`/`REQUIRE` (matching `test_parallel_for.cpp`'s style, not the BDD style used
in `serde` tests). Per function, written test-first (reproducer red before implementation,
per project workflow rule):

**`parallel_chunks_for_each`**
- Below-threshold input runs as a single serial call (verify via a call counter reaching 1).
- Above-threshold input: every index mutated exactly once, matching `parallel_for_each`'s
  existing "exactly once per element" test shape.
- Explicit `chunk_count` is respected (verify observed chunk boundaries or call count).
- `min_chunk_size == 0` always parallelizes even for small input.
- Empty input (`count == 0`) is a no-op.

**`parallel_chunks_transform`**
- Output written at the correct offsets for each chunk; full output matches a serially
  computed reference.
- Below-threshold falls back to one call covering the whole input/output.

**`parallel_chunks_reduce`**
- Sum-like reduction across chunks matches a plain serial `std::accumulate` reference.
- Below-threshold path matches `combine(init, f(whole input))` exactly.
- Empty input returns `init` unchanged, `f`/`combine` never invoked (assert via call counters).

## Open items for the implementation plan

- Exact `min_chunk_size` parameter type/units are element counts (matching CDFpp's
  `_thread_if_needed<min_chunk_size>` template parameter, which is also an element count in
  its one real usage) — no byte-size variant is in scope.
- Whether `chunk_count`/`min_chunk_size` should have `[[nodiscard]]`-style doc comments
  matching `parallel_for.hpp`'s existing comment density — follow the existing file's
  convention when writing the plan.
