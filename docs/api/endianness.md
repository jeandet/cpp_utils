# endianness

Byte-order-aware decode/encode helpers for reading and writing fundamental types to/from raw
byte buffers, with the target byte order selected either at compile time (via a tag type) or at
runtime (via an `Endianness` enum value). Used internally by `serde/`, but usable standalone for
any wire-format or file-format parsing that needs explicit little/big-endian handling.

```cpp
#include <endianness/endianness.hpp>
```

All symbols below live in `cpp_utils::endianness`.

## Endianness tags: `little_endian_t`, `big_endian_t`, `host_endianness_t`

```cpp
struct little_endian_t {};
struct big_endian_t {};

using host_endianness_t = /* little_endian_t or big_endian_t, matching the host */;
```

Empty tag types used as template arguments to `decode`/`encode`/`decode_v`/`encode_v` to say
"this buffer is/should be in this byte order". `host_endianness_t` is an alias for whichever of
the two matches the compiling host; `decode`/`encode` compare their source/destination tag
against it to decide whether a byte swap is needed.

Two small traits classify a tag type at compile time, and a runtime accessor mirrors
`host_endianness_t` as an `Endianness` value:

```cpp
template <typename endianness_t> inline constexpr bool is_little_endian_v;
template <typename endianness_t> inline constexpr bool is_big_endian_v;

inline Endianness host_endianness_v();
```

```cpp
static_assert(cpp_utils::endianness::is_little_endian_v<cpp_utils::endianness::little_endian_t>);
```

## `host_is_big_endian` / `host_is_little_endian`

```cpp
inline constexpr bool host_is_big_endian;
inline constexpr bool host_is_little_endian;
```

Compile-time constants describing the compiling host's native byte order (derived from
`std::endian::native`). Exactly one of the two is `true`; mixed-endian hosts are rejected with a
`static_assert` at header-inclusion time. Handy for picking the "opposite of host" tag in tests,
as done here:

```cpp
using namespace cpp_utils::endianness;
using target_endianness = std::conditional_t<host_is_big_endian, little_endian_t, big_endian_t>;
```

## `Endianness`

```cpp
enum class Endianness
{
    unknown,
    big,
    little
};
```

Runtime counterpart to the tag types, for call sites that only know the desired byte order at
run time (e.g. a format flag read from a file header). Passed to the runtime-dispatch overloads
of `decode`/`encode` below. `Endianness::unknown` is treated as little-endian by those overloads,
matching `serde`'s own default for composites that don't opt into `big_endian_t`.

## `decode`

```cpp
template <typename src_endianess_t, typename T, typename U>
[[nodiscard]] T decode(const U* input);

template <typename T, typename U>
[[nodiscard]] T decode(Endianness src_endianness, const U* input);
```

Reads a `T` out of the `sizeof(T)` bytes starting at `input`, byte-swapping only if
`src_endianess_t` (or, for the runtime overload, `src_endianness`) differs from the host's byte
order; single-byte `T` is never swapped. `input` must be non-null; it doesn't need to satisfy
`T`'s alignment. The runtime overload dispatches to the tag-based one internally, so both give
identical results:

```cpp
using namespace cpp_utils::endianness;

REQUIRE(0x01020304 == decode<big_endian_t, uint32_t>("\1\2\3\4"));

REQUIRE(decode<little_endian_t, uint32_t>("\1\2\3\4")
    == decode<uint32_t>(Endianness::little, "\1\2\3\4"));
```

## `encode`

```cpp
template <typename dst_endianess_t, typename T>
inline void encode(T value, char* output);

template <typename T>
inline void encode(Endianness dst_endianness, T value, char* output);
```

Writes `value` into the `sizeof(T)` bytes starting at `output`, byte-swapping only if
`dst_endianess_t` (or `dst_endianness`) differs from the host's byte order. `output` must point
at a buffer of at least `sizeof(T)` bytes. Mirrors `decode`: the runtime overload dispatches to
the tag-based one, so both produce identical bytes:

```cpp
using namespace cpp_utils::endianness;

char buffer[4];
encode<big_endian_t>(0x01020304u, buffer);
// buffer == {0x01, 0x02, 0x03, 0x04}

char runtime_buffer[4];
encode<uint32_t>(Endianness::big, 0x01020304, runtime_buffer);
// runtime_buffer == buffer
```

## `decode_v`

```cpp
template <typename src_endianess_t, typename value_t>
inline void decode_v(value_t* data, std::size_t size);

template <typename src_endianess_t, typename value_t>
inline void decode_v(const auto* data, std::size_t size, value_t* output);
```

Bulk/array form of `decode`. The first overload decodes `size` elements of `value_t` in place at
`data` (`size` counts `value_t` elements). The second overload reads from a possibly
differently-typed, possibly misaligned source pointer `data` — `size` counts elements of
`*data`'s type, internally converted to a `value_t` element count as
`size * sizeof(*data) / sizeof(value_t)` — and writes the decoded `value_t`s to `output`; reads
and writes both go through unaligned load/store, so `data` need not satisfy `value_t`'s
alignment (e.g. a byte buffer sliced at an arbitrary offset):

```cpp
using namespace cpp_utils::endianness;
using target_endianness = std::conditional_t<host_is_big_endian, little_endian_t, big_endian_t>;

std::vector<uint8_t> input{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
std::vector<uint8_t> output(16);
decode_v<target_endianness>(input.data(), std::size(input), reinterpret_cast<uint16_t*>(output.data()));
// output == {2,1,4,3,6,5,8,7,10,9,12,11,14,13,16,15}
```

```cpp
uint16_t values[2] = {0x0201, 0x0403};
decode_v<target_endianness>(values, 2);
// values == {0x0102, 0x0304}
```

## `encode_v`

```cpp
template <typename dst_endianess_t, typename value_t>
inline void encode_v(const value_t* data, std::size_t count, char* output);
```

Bulk/array form of `encode`: writes `count` elements of `data` into `output` as
`count * sizeof(value_t)` bytes, byte-swapping each element only if `dst_endianess_t` differs
from the host's byte order.

```cpp
using namespace cpp_utils::endianness;

std::vector<uint16_t> values{0x0102, 0x0304};
std::vector<char> output(4);
encode_v<big_endian_t>(values.data(), values.size(), output.data());
// output == {0x01, 0x02, 0x03, 0x04}
```

## Implementation note

The actual byte swap is `__builtin_bswap16/32/64` (GCC/Clang) or `_byteswap_ushort/ulong/uint64`
(MSVC) by default. When built under C++23 or later with a standard library implementing
`std::byteswap` (`__cpp_lib_byteswap` defined), it's used instead — a path that's dead under this
project's own hard-pinned `cpp_std=c++20` build.
