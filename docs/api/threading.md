# threading

Parallel helpers built on top of a process-wide `BS::light_thread_pool` singleton
(from the [bshoshany-thread-pool](https://github.com/bshoshany/thread-pool) library).

**Opt-in required.** This module is gated behind the Meson option `-Dthreading=true`
(off by default) and is deliberately **not** part of `cpp_utils_dep` — a plain consumer
of the library never pulls in the thread-pool dependency. If your code includes
`threading/parallel_for.hpp` or `threading/parallel_chunks.hpp`, enable that option and
wire up the `bshoshany-thread-pool` dependency yourself.

Two headers, two different callback shapes:

- `threading/parallel_for.hpp` — `parallel_for`/`parallel_for_each`, callback receives
  a bare index (or element reference).
- `threading/parallel_chunks.hpp` — `parallel_chunks_for_each`/`_transform`/`_reduce`,
  callback receives a contiguous `std::span` over a slice of real data, not an index.

## `parallel_for` / `parallel_for_each`

```cpp
#include <threading/parallel_for.hpp>
```

```cpp
template <typename F>
void parallel_for(std::size_t count, F&& f);

template <typename Container, typename F>
void parallel_for_each(Container& items, F&& f);
```

`parallel_for(count, f)` calls `f(index)` exactly once for every `index` in
`[0, count)`, distributing the calls across the shared pool and blocking until all of
them complete. For `count == 0` it's a no-op; for `count == 1` it calls `f(0)` inline
without touching the pool at all.

`parallel_for_each(items, f)` is a convenience wrapper: it calls `parallel_for` over
`std::size(items)` and forwards `f(items[i])` — `f` receives each element **by
reference**, not an index.

The pool itself (`pool()`) is a lazily-initialized singleton sized to
`std::thread::hardware_concurrency()`, created on first use and shared by every call to
`parallel_for`/`parallel_for_each` (and by `parallel_chunks_*` below) for the lifetime of
the process.

```cpp
#include <threading/parallel_for.hpp>
#include <atomic>
#include <vector>

using namespace cpp_utils::threading;

std::vector<std::atomic<int>> hits(1000);
parallel_for(hits.size(), [&](std::size_t i) { hits[i]++; });

std::vector<int> items(200, 0);
parallel_for_each(items, [](int& v) { v = 1; });
```

## `parallel_chunks_for_each` / `parallel_chunks_transform` / `parallel_chunks_reduce`

```cpp
#include <threading/parallel_chunks.hpp>
```

```cpp
template <typename Input, typename F>
void parallel_chunks_for_each(
    Input& input, std::size_t min_chunk_size, F&& f, std::size_t chunk_count = 0);

template <typename Input, typename Output, typename F>
void parallel_chunks_transform(const Input& input, Output output,
    std::size_t min_chunk_size, F&& f, std::size_t chunk_count = 0);

template <typename Input, typename T, typename F, typename BinaryOp>
T parallel_chunks_reduce(const Input& input, T init, std::size_t min_chunk_size,
    F&& f, BinaryOp&& combine, std::size_t chunk_count = 0);
```

Instead of one call per index, these split `input` into `chunk_count` contiguous
blocks (using the pool's own block-splitting — `detach_blocks`/`submit_blocks` — rather
than hand-rolled chunk math) and invoke the callback **once per chunk**, each time with
a `std::span` over that chunk's real elements. `chunk_count == 0` (the default) uses the
pool's thread count.

**Threshold gate.** Before dispatching, each function compares `count` (the number of
elements in `input`) against `min_chunk_size * effective_chunk_count`:
- if `count < min_chunk_size * effective_chunk_count`, the pool is skipped entirely and
  the callback is invoked exactly once, serially, with a span over the **whole** input;
- otherwise, the work is split across the pool as described above.

Pass `min_chunk_size = 0` to disable the gate and always parallelize. All three
functions are a no-op (`init` returned untouched, callbacks never invoked) when `input`
is empty.

### `parallel_chunks_for_each`

`f(std::span<T> chunk)` — mutates each chunk's elements in place.

```cpp
#include <threading/parallel_chunks.hpp>
#include <span>
#include <vector>

using namespace cpp_utils::threading;

std::vector<int> items(1000, 0);
parallel_chunks_for_each(items, std::size_t { 1 },
    [](std::span<int> chunk)
    {
        for (auto& v : chunk)
            v += 1;
    });
```

### `parallel_chunks_transform`

`f(std::span<const T> chunk, Output out)` — `out` is `output + start`, the pointer
offset matching where `chunk` begins in `input`. `output` must point to storage for at
least `std::ranges::size(input)` elements; this is the caller's responsibility and is
not validated.

```cpp
#include <threading/parallel_chunks.hpp>
#include <numeric>
#include <span>
#include <vector>

using namespace cpp_utils::threading;

std::vector<int> input(1000);
std::iota(input.begin(), input.end(), 0);
std::vector<int> output(input.size(), -1);

parallel_chunks_transform(input, output.data(), std::size_t { 1 },
    [](std::span<const int> chunk, int* out)
    {
        for (std::size_t i = 0; i < chunk.size(); ++i)
            out[i] = chunk[i] * 2;
    });
```

### `parallel_chunks_reduce`

`f(std::span<const T> chunk)` reduces one chunk to a single partial result;
`combine(accumulator, partial)` folds each chunk's partial into the running
accumulator, starting from `init`. Below the threshold this collapses to
`combine(init, f(whole input))`.

```cpp
#include <threading/parallel_chunks.hpp>
#include <numeric>
#include <span>
#include <vector>

using namespace cpp_utils::threading;

std::vector<int> input(1000);
std::iota(input.begin(), input.end(), 1);

auto sum = parallel_chunks_reduce(
    input, 0, std::size_t { 1 },
    [](std::span<const int> chunk) { return std::accumulate(chunk.begin(), chunk.end(), 0); },
    [](int a, int b) { return a + b; });
```
