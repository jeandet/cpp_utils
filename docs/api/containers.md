# containers

Small, dependency-light building blocks: a non-value-initializing vector for perf-sensitive
buffers, a vector-backed associative container, a handful of container algorithms, N-D flat-index
helpers, and the type-trait machinery `contains`/`index_of` rely on. Everything lives in
`namespace cpp_utils::containers`.

## no_init_vector

```cpp
#include <containers/no_init_vector.hpp>
```

`no_init_vector<T>` is an alias for `std::vector<T, default_init_allocator<T>>`.
`default_init_allocator` overrides the allocator's `construct(ptr)` (no-argument) call to use
placement **default**-initialization (`::new (ptr) U;`) instead of the value-initialization
(`::new (ptr) U();`) `std::allocator` uses. For scalar/trivial `T` (`int`, `double`, POD structs),
default-initialization is a no-op — the memory is left exactly as the allocator returned it,
i.e. **garbage**, not zeroed.

This only affects construction with no supplied value:

```cpp
template <typename T>
using no_init_vector = std::vector<T, default_init_allocator<T>>;
```

```cpp
cpp_utils::containers::no_init_vector<double> buffer(4096);
// buffer[i] is uninitialized memory here - reading any element before writing it is UB.
std::fread(buffer.data(), sizeof(double), buffer.size(), file);
// buffer now holds real data and is safe to read.
```

`no_init_vector<T>(n)` / `v.resize(n)` leave every element uninitialized for trivial `T`.
`push_back(x)`, `resize(n, x)`, and any other construct-from-a-value call go through the normal
`construct(ptr, args...)` overload and behave exactly like `std::vector` — only bare
default-construction skips initialization. For non-trivial `T` (a class with a real default
constructor), default- and value-initialization coincide, so there is no observable difference
from `std::vector<T>`.

Use `no_init_vector` only for buffers you are about to fully overwrite (file reads, decode
targets, scratch buffers) — never read an element before writing it.

On platforms with `<sys/mman.h>`, the allocator also takes a `posix_memalign` + `MADV_HUGEPAGE`
fast path for allocations of trivially/nothrow-default-constructible `T` that are at least 4 MiB
(2 × the allocator's 2 MiB `page_size` constant); smaller or non-trivial allocations fall back to
`malloc`/the base allocator.

Because `no_init_vector<T>` and `std::vector<T>` are different `std::vector` instantiations
(different allocator template argument), the standard library's own `operator==` — which requires
both sides to share the same allocator type — does not compare them. This header adds free
`operator==` overloads (both argument orders) so a `no_init_vector<T>` can be compared directly
against a `std::vector<T>`, plus a `std::size(no_init_vector<T> const&)` overload.

## nomap

```cpp
#include <containers/nomap.hpp>
```

`nomap<Key, T>` is a vector-backed associative container with a `std::map`/`std::unordered_map`
style API subset. Lookups (`find`, `at`, `operator[]`, `count`) are **linear scans** over the
backing `std::vector` — it trades lookup complexity for cache-friendly storage and is intended for
small maps. `erase` is O(1) but swaps the erased element with the last one before popping it, so
it does **not** preserve the relative order of the remaining elements.

### `nomap_node<Key, T>`

The element type: a `std::pair<Key, T>` subclass with named accessors and structured-binding
support (`tuple_size`/`tuple_element` specializations, so `auto [k, v] = *it;` works).

```cpp
template <typename Key, typename T>
struct nomap_node : public std::pair<Key, T>
{
    [[nodiscard]] auto& key() const noexcept;
    [[nodiscard]] auto& mapped() const noexcept;
    [[nodiscard]] bool empty() const noexcept; // true only for a default-constructed node
};
```

```cpp
cpp_utils::containers::nomap_node<int, int> a { 10, 1 };
cpp_utils::containers::nomap_node<int, int> b { 20, 2 };

using std::swap;
swap(a, b); // a == {20, 2}, b == {10, 1}
```

### `nomap<Key, T>`

```cpp
template <typename Key, typename T>
struct nomap
{
    [[nodiscard]] T& at(const key_type& key);              // throws std::out_of_range if absent
    [[nodiscard]] const T& at(const key_type& key) const;
    [[nodiscard]] T& operator[](const key_type& key);       // inserts default T{} if absent
    [[nodiscard]] auto find(const key_type& key);            // -> iterator, end() if absent
    [[nodiscard]] size_type count(const Key& key) const;     // 0 or 1
    template <typename Kt, class... Args>
    std::pair<iterator, bool> emplace(Kt&& key, Args&&... args);
    [[nodiscard]] value_type extract(const_iterator position);
    [[nodiscard]] value_type extract(const key_type& key);   // empty node if key absent
    iterator erase(iterator position);
    const_iterator erase(const_iterator position);
    iterator erase(const_iterator first, const_iterator last);
    // size, empty, max_size, clear, reserve, swap, begin/end/rbegin/rend/c*
};
```

```cpp
cpp_utils::containers::nomap<int, int> m;
m[1] = 10;
m[2] = 20;

m.at(1);              // 10
m.count(3);            // 0
m.find(2) != m.end();  // true

auto [it, inserted] = m.emplace(3, 30); // inserted == true, m[3] == 30

auto node = m.extract(1);  // removes key 1, returns its node
node.key();                 // 1
node.mapped();               // 10

m.erase(m.find(2));   // O(1): swaps element 2 with the back, then pops it
```

`operator[]` requires `T` to be default-constructible (a fresh `mapped_type{}` is inserted on a
miss). `at`/`operator[]` return `T&`/`const T&`; the container's declared `mapped_type` is the
cv/ref-stripped `T`.

## algorithms

```cpp
#include <containers/algorithms.hpp>
```

### `contains`

```cpp
template <class T1, class T2>
auto contains(const T1& container, const T2& value);
```

`true` if `value` is found in `container` (`is_container_v<T1>` is `static_assert`ed). Uses
`container.find(value)` when the container has a `find` member (e.g. `std::set`, `std::map`),
otherwise falls back to `std::find` over `[cbegin, cend)`.

```cpp
using namespace cpp_utils::containers;
std::vector<double> v { 1., 2., 3. };
contains(v, 1.);              // true
contains(std::string { "hello" }, 'e'); // true
```

### `index_of`

```cpp
template <class T1, class T2>
auto index_of(const T1& container, const T2& value);
```

Distance from `begin()` to the first element equal to `value` (`is_sequence_container_v<T1>` is
`static_assert`ed); returns `std::size(container)` (i.e. the end-position index) when not found.
If elements aren't directly comparable to `value` (e.g. `container` holds `std::shared_ptr<X>`
and `value` is a raw `X*`), it falls back to comparing `value == item.get()`.

```cpp
std::vector<double> v { 1., 2., 3. };
index_of(v, 2.);     // 1
index_of(v, 1111.);  // 3 (== std::size(v), i.e. "not found")

std::vector<std::shared_ptr<int>> ptrs { std::make_shared<int>(1), std::make_shared<int>(2) };
index_of(ptrs, ptrs[1]);       // 1, compares shared_ptr == shared_ptr
index_of(ptrs, ptrs[1].get()); // 1, compares value == item.get()
```

### `split`

```cpp
void split(const types::concepts::contiguous_sequence_container auto& input,
    types::concepts::sequence_container auto&& output, const auto splitVal);
```

Splits `input` into contiguous runs separated by `splitVal`, appending each run (a value of
`input`'s own type) to `output`. Consecutive separators, and leading/trailing separators, do not
produce empty entries.

```cpp
std::string path { "/part1/part2/part3" };
std::vector<std::string> parts;
split(path, parts, '/'); // { "part1", "part2", "part3" }

std::vector<int> nums { 1, 2, 3, 4, 5, 6 };
std::list<std::vector<int>> chunks;
split(nums, chunks, 4); // { {1, 2, 3}, {5, 6} }
```

### `join`

```cpp
template <class input_t, class item_t>
auto join(const input_t& input, const item_t& sep);
```

Inverse of `split`: concatenates `input`'s elements (each of `input`'s element type, e.g.
`std::string` for a `std::vector<std::string>`) with `sep` inserted between them. Returns a
default-constructed empty result for an empty `input`.

```cpp
std::vector<std::string> parts { "part1", "part2", "part3" };
join(parts, '/'); // "part1/part2/part3"

std::list<std::vector<int>> chunks { { 1, 2, 3 }, { 5, 6 } };
join(chunks, 4); // std::vector<int> { 1, 2, 3, 4, 5, 6 }
```

### `min` / `max`

```cpp
template <typename T> auto min(const T& v); // -> std::optional<element type>
template <typename T> auto max(const T& v); // -> std::optional<element type>
```

`std::nullopt` for an empty `v`, otherwise the smallest/largest element.

```cpp
*min(std::vector { 1, 2, 3 }); // 1
*max(std::vector { 1, 2, 3 }); // 3
min(std::vector<int> {});      // std::nullopt
```

### `broadcast`

```cpp
template <typename T, typename U, typename... V>
void broadcast(const T& collection, U fptr, V&&... parameters); // const-collection overload
template <typename T, typename U, typename... V>
void broadcast(T& collection, U fptr, V&&... parameters);       // mutable-collection overload
```

Calls `std::invoke(fptr, item, parameters...)` for every `item` in `collection` — `fptr` can be a
free function, a callable, or a pointer-to-member (invoked on each `item`). The mutable overload
is selected for a non-`const` `collection`, giving `fptr` a mutable reference to each `item`.

```cpp
struct Widget { void add(int v) { total += v; } int total = 0; };
std::vector<Widget> ws(10);
broadcast(ws, &Widget::add, 1); // ws[i].total == 1 for every i
```

### `flat_size`

```cpp
template <typename T>
auto flat_size(const T& shape); // -> std::size_t
```

Product of a shape/extents container's values — the element count of an N-D array given its
per-dimension sizes. `0` for an empty `shape`.

```cpp
flat_size(std::vector<std::size_t> { 2, 3, 4 }); // 24
flat_size(std::vector<std::size_t> {});          // 0
```

## nd_shape

```cpp
#include <containers/nd_shape.hpp>
```

Arbitrary-dimension-count flat indexing and axis permutation for row-major or column-major
buffers. `flat_index`/`permute_axes` are plain `O(ndim)` loops — no compile-time cap on the
number of dimensions.

```cpp
enum class array_order
{
    row_major,    // C order: last dimension varies fastest
    column_major  // Fortran order: first dimension varies fastest
};
```

### `flat_index`

```cpp
template <typename IndexRange, typename ShapeRange>
std::size_t flat_index(const IndexRange& indices, const ShapeRange& shape,
    array_order order = array_order::row_major);
```

Flat offset of the N-D point `indices` into a buffer of the given per-dimension `shape`.

```cpp
using namespace cpp_utils::containers;
const std::vector<std::size_t> shape { 2, 3 }; // a 2x3 array

flat_index(std::vector<std::size_t> { 1, 0 }, shape);                            // 3  (row-major, default)
flat_index(std::vector<std::size_t> { 1, 0 }, shape, array_order::column_major); // 1
```

Row-major walks the buffer with the *last* index varying fastest (standard C array layout);
column-major walks it with the *first* index varying fastest (Fortran layout) — same shape,
same indices, different flat offset.

### `permute_axes`

```cpp
template <typename Container>
Container permute_axes(const Container& data, const std::vector<std::size_t>& shape,
    const std::vector<std::size_t>& permutation);
```

Reorders a row-major N-D buffer `data` so that axis `permutation[k]` of `data`/`shape` becomes
axis `k` of the result. `permutation` must be a permutation of `[0, shape.size())`; a full-axis
reversal (e.g. `{2, 1, 0}` for a 3-D shape) is an N-D transpose. Favors clarity over throughput —
element-by-element copy, no contiguous-run fast path.

```cpp
using namespace cpp_utils::containers;
// row-major 2x3 [[1,2,3],[4,5,6]]
const std::vector<int> data { 1, 2, 3, 4, 5, 6 };
const std::vector<std::size_t> shape { 2, 3 };

permute_axes(data, shape, std::vector<std::size_t> { 1, 0 });
// row-major 3x2 [[1,4],[2,5],[3,6]] -> { 1, 4, 2, 5, 3, 6 }

permute_axes(data, shape, std::vector<std::size_t> { 0, 1 }); // identity permutation: == data
```

`permute_axes` calls `flat_size` (see [algorithms](#algorithms)) to size its result buffer.

## traits

```cpp
#include <containers/traits.hpp>
```

Type traits used to constrain the algorithms above (`contains`/`index_of` `static_assert` on
these) and to recognize standard container categories. These are plain C++17-style
`std::true_type`/`std::false_type` traits, distinct from the C++20 concepts of the same shape in
`types/concepts.hpp` (`container`, `sequence_container`, `contiguous_sequence_container` — used
by `split`) — the two are related but **not** interchangeable: e.g.
`is_container_v<std::forward_list<int>>` is `true` (this header special-cases `std::list`/
`std::forward_list` to count as containers regardless of the method-detection checks below), while
`types::concepts::container` requires a `.size()` method that `std::forward_list` doesn't have and
so rejects it.

```cpp
template <class T>
static inline constexpr bool is_container_v = /* see below */;

template <class T>
static inline constexpr bool is_sequence_container_v = /* see below */;

template <class T>
static inline constexpr bool is_std_array_v = /* true iff T is std::array<U, N> */;
```

`is_container_v<T>` is `true` for any `std::list`/`std::forward_list`, or any `T` that has
`value_type`, `reference`, `const_reference`, `iterator`, `const_iterator`, `difference_type`,
`size_type`, `begin`/`end`/`cbegin`/`cend`/`swap`/`size`/`empty` — the member-detection traits
(`has_size_method<T>`, `has_begin_method<T>`, …) come from `types/detectors.hpp`.

`is_sequence_container_v<T>` additionally requires an `insert` method (or is `true` unconditionally
for `std::list`, `std::forward_list`, and `std::array`, which are treated as sequence containers
regardless).

```cpp
using namespace cpp_utils::containers;
static_assert(is_container_v<std::vector<int>>);
static_assert(is_container_v<std::map<int, int>>);
static_assert(is_sequence_container_v<std::vector<int>>);
static_assert(is_sequence_container_v<std::array<int, 3>>);
static_assert(is_std_array_v<std::array<int, 3>>);
static_assert(!is_std_array_v<std::vector<int>>);
```

This header also exposes narrower std-type-detection traits used to build the two above —
`is_std_list<T>`, `is_std_forward_list<T>`, `is_std_vector<T>`, `is_std_deque<T>`,
`is_std_basic_string<T>` — each a `std::true_type`/`std::false_type`-derived trait recognizing
that specific standard container template, generated via the `IS_TEMPLATE_T` macro (from
`types/detectors.hpp`).
