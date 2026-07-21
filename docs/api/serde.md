# `serde` — binary (de)serialization

`serde` turns an ordinary C++20 aggregate into a description of an on-wire binary layout. You
write a plain struct whose members appear in wire order; `serde::deserialize<T>` reads bytes into
an instance of it, `serde::serialize` writes an instance back out to bytes, and `serde::runtime_size`
computes how many bytes `serialize` would produce without writing anything. There is no codegen and
no per-type boilerplate: fundamental/enum members are decoded/encoded directly (with
endianness handled per struct), nested composite members recurse field-by-field, and members that
need something other than "read `sizeof(T)` bytes" (variable-length arrays, padding, bounded
strings, fixed-point values, discarded/reserved space) are declared with a wrapper type from
`special_fields.hpp` instead of a plain type.

The whole module is built on the reflection machinery in `reflexion/reflection.hpp`, which is what
lets `deserialize`/`serialize`/`runtime_size` walk an arbitrary aggregate's fields without you
writing a visitor for each type.

```cpp
#include <serde/serde.hpp>
```

## `deserialize`

Two overloads exist: a top-level convenience form that allocates and returns a new `T`, and an
in-place form (generated alongside the field-splitting machinery) that fills an existing object and
returns the offset just past what it consumed. The in-place form is what the top-level form calls
internally, and it's also what's used to recurse into nested composite fields — but it's a plain
public overload of `deserialize`, so you can call it directly too, e.g. to walk consecutive records
in one buffer.

```cpp
template <typename composite_t, typename context_t = no_context>
constexpr composite_t deserialize(auto&& input, std::size_t offset = 0,
    const context_t& context = context_t {});

// in-place / recursive overload
template <typename composite_t, typename context_t = no_context>
constexpr std::size_t deserialize(composite_t& out, auto&& input, std::size_t offset = 0,
    const context_t& context = context_t {});
```

`input` is anything that can back a `std::span` over its bytes — `std::vector<uint8_t>`,
`std::string`, `std::array<uint8_t, N>`, `io::memory_mapped_file`, `io::buffer_view`, etc. — or,
when recursing into composite fields, whatever opaque object the top-level call was given
(forwarded through unchanged).

```cpp
struct two_chars { char a; char b; };
std::string buffer { "cd" };
auto s = cpp_utils::serde::deserialize<two_chars>(buffer);
// s.a == 'c', s.b == 'd'
```

Reading several fixed-format records back-to-back from the same buffer, using the in-place
overload's returned offset to advance:

```cpp
struct rec { uint8_t a; uint8_t b; };
std::vector<uint8_t> buffer { 1, 2, 3, 4 };
std::size_t offset = 0;
std::vector<rec> records;
while (offset < buffer.size())
{
    rec r;
    offset = cpp_utils::serde::deserialize(r, buffer, offset);
    records.push_back(r);
}
```

## `serialize`

```cpp
template <typename composite_t, byte_sink sink_t, typename context_t = no_context>
constexpr std::size_t serialize(const composite_t& value, sink_t& sink,
    std::size_t offset = 0, const context_t& context = context_t {});
```

Writes `value` into `sink` starting at `offset`, growing `sink` as needed (see [`byte_sink`](#byte_sink)
below), and returns the offset one past the last byte written. Because the sink is only grown, never
shrunk, `sink.size()` isn't necessarily the "real" written length when `offset` is non-zero or the
sink was pre-sized — use the returned offset for that.

```cpp
struct two_chars { char a; char b; };
two_chars value { 'c', 'd' };
std::vector<uint8_t> sink;
auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
// end_offset == 2, sink == {'c', 'd'}
```

## `runtime_size`

```cpp
template <typename composite_t, typename context_t = no_context>
constexpr std::size_t runtime_size(const composite_t& value, const context_t& context = context_t {});
```

Computes the exact byte length `serialize(value, sink, 0, context)` would produce, without touching
any sink — handy for filling in a record's own size header before writing it, or for pre-sizing a
buffer.

```cpp
struct struct2 { struct { char a; char b; } s; char c; };
struct2 value { { 'a', 'b' }, 'c' };
cpp_utils::serde::runtime_size(value); // == 3
```

For dynamic-size fields, `runtime_size` (like `serialize`) uses the field's actual in-memory
`.size()` — it does **not** re-derive a sibling length/count field from that size. If a composite
has both a count field and a `dynamic_array`/`dynamic_array_bytes` field describing its own
`field_size()`, you're responsible for keeping the two in sync yourself before calling `serialize`
or `runtime_size`:

```cpp
struct with_table
{
    uint16_t count_bytes;
    cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;
    std::size_t field_size(const decltype(values)&) const { return count_bytes; }
};

with_table value {};
value.count_bytes = 4;      // must match values' byte length yourself
value.values.resize(2);
value.values[0] = 1;
value.values[1] = 2;
cpp_utils::serde::runtime_size(value); // == 6 (2 header bytes + 4 payload bytes)
```

(This only matters when you build the object by hand. Round-tripping — `deserialize` then
`serialize` — stays consistent automatically, since `count_bytes` is itself an ordinary field that
gets read from and written back to the wire unchanged.)

## The `context` mechanism

`deserialize`, `serialize` and `runtime_size` all accept an optional `context` object, threaded
unchanged into every recursive call. It exists for one purpose: resolving a dynamic field's size
when that size lives outside the field's immediate parent struct.

```cpp
struct no_context { };
```

`no_context` is the default when you don't pass one. It carries no data and most structs never need
to reference it.

### How `field_size` resolution works

`dynamic_array<ID, T>` and `dynamic_array_bytes<ID, T>` fields get their element/byte count by
calling back into the parent composite's `field_size` member function. The resolution
(`details::resolve_field_size`) prefers a context-aware overload if the parent declares one, and
falls back to the context-less one otherwise:

```cpp
template <typename parent_t, typename field_t, typename context_t>
constexpr auto resolve_field_size(const parent_t& parent, const field_t& field, const context_t& context)
{
    if constexpr (requires { parent.field_size(field, context); })
        return parent.field_size(field, context);
    else
        return parent.field_size(field);
}
```

So a struct only needs the context-aware overload if its size actually depends on something outside
itself; every other struct in the same object graph can keep the simple one-argument `field_size`,
and the two forms can be mixed freely across different composites reached during the same
`deserialize`/`serialize`/`runtime_size` call.

### When you need a custom context type

Use a context when a dynamic field's size is only known from data that isn't a sibling field of the
same struct — e.g. it comes from an ancestor record, or from information the caller has out of band.
There's no required shape for a context type beyond being an ordinary copyable type you can
construct; it's just data your `field_size` overload chooses to read from:

```cpp
struct dims_context { int32_t num_dims; };

struct rvdr_like
{
    cpp_utils::serde::dynamic_array_bytes<0, int32_t> dim_varys;

    std::size_t field_size(const decltype(dim_varys)&, const dims_context& ctx) const
    {
        return static_cast<std::size_t>(ctx.num_dims) * sizeof(int32_t);
    }
};

dims_context ctx { 2 };
auto s = cpp_utils::serde::deserialize<rvdr_like>(buffer, 0, ctx);
```

The same `ctx` is forwarded down into every nested composite field too, so a deeply nested dynamic
field can still see it as long as every intermediate `field_size` overload (if any) forwards it
along or simply doesn't need it.

## `byte_sink`

```cpp
template <typename T>
concept byte_sink = requires(T t, std::size_t n) {
    { t.resize(n) };
    { t.data() };
    { t.size() } -> std::convertible_to<std::size_t>;
};
```

Anything `resize`/`data`/`size`-shaped qualifies — `std::vector<uint8_t>` and `std::string` both do
out of the box. `serialize` only ever grows the sink (`if (sink.size() < needed) sink.resize(needed);`),
never shrinks it, so a pre-sized or reused sink keeps whatever tail bytes it already had past the
written region.

## Per-struct endianness

By default every composite is decoded/encoded little-endian. Opt a struct into big-endian by giving
it a nested `endianness` type alias:

```cpp
struct big_endian
{
    using endianness = cpp_utils::endianness::big_endian_t;
    uint32_t a;
    uint16_t b;
};
```

This is resolved per composite (`endianness_t<composite_t>`, defaulting to
`endianness::little_endian_t` when the alias is absent), so different structs — including a struct
and the composites nested inside it — can each declare their own byte order independently. There's
also an `is_big_endian_v<composite_t>` trait if you need to branch on it.

## Field wrappers (`special_fields.hpp`)

Plain fundamental/enum members and plain nested composites need no wrapper — they're read/written
directly. Everything else (fixed arrays, variable-length arrays, padding, bounded strings,
fixed-point values, discarded fields) is declared with one of the following wrapper types.

Every wrapper is tagged `do_not_split` (`reflexion::do_not_split_t`) so the reflection layer treats
it as one opaque field instead of trying to structured-bind into its internals; the dynamic ones are
additionally tagged `dyn_size_field_tag` so `const_size_field<T>` (and therefore
`composite_have_const_size`) correctly reports that the enclosing composite doesn't have a
compile-time-known size.

### `static_array<T, N>`

Fixed-size array field: always exactly `N` elements of `T`, no size resolution needed.

```cpp
struct array_members
{
    cpp_utils::serde::static_array<uint16_t, 2> a;
    uint16_t b;
};
std::array<uint8_t, 6> buffer { 1, 0, 2, 0, 3, 0 };
auto s = cpp_utils::serde::deserialize<array_members>(buffer);
// s.a[0] == 1, s.a[1] == 2, s.b == 3
```

### `dynamic_array<ID, T>`

Resizable array of `T`, sized via `parent.field_size(field[, context])` interpreted as an **element
count**. `ID` is a `std::size_t` template parameter used purely to give two `dynamic_array` fields
of the same `T` distinct C++ types, so a parent composite can overload `field_size` per field even
when their element types match.

```cpp
struct with_table
{
    uint16_t element_count;
    cpp_utils::serde::dynamic_array<0, uint16_t> values;

    std::size_t field_size(const decltype(values)&) const { return element_count; }
};
// element_count = 2 (LE u16), followed by 2 u16 elements
std::vector<uint8_t> buffer { 2, 0, 1, 0, 2, 0 };
auto s = cpp_utils::serde::deserialize<with_table>(buffer);
// s.values.size() == 2, s.values[0] == 1, s.values[1] == 2
```

Element type `T` can itself be a compound (e.g. a small struct); each element is then deserialized
recursively rather than bulk-decoded.

### `dynamic_array_bytes<ID, T>`

Same as `dynamic_array<ID, T>` (in fact derives from it), except the value returned by
`field_size(field[, context])` is interpreted as a **byte** length, which is then divided by
`sizeof(T)` to get the element count.

```cpp
struct with_table
{
    uint16_t count_bytes;
    cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;

    std::size_t field_size(const decltype(values)&) const { return count_bytes; }
};
std::vector<uint8_t> buffer { 4, 0, 1, 0, 2, 0 }; // count_bytes = 4 -> 2 uint16_t elements
auto s = cpp_utils::serde::deserialize<with_table>(buffer);
// s.values.size() == 2
```

### `dynamic_array_until_eof<T>`

`= dynamic_array<SIZE_MAX, T>` — consumes whatever is left in the buffer instead of resolving a
count via `field_size`. Behavior on load depends on whether `T` is constant-size
(`const_size_field<T>`, defined in this header):

- constant-size `T` (fundamental, or a compound whose size is known at compile time): the remaining
  byte count is divided by `T`'s size to get an element count, then that many elements are read.
- dynamic-size `T` (must be a compound type — enforced by `static_assert`): elements are decoded one
  at a time and appended (`emplace_back`) until the offset reaches the end of the buffer — used for
  a trailing run of self-describing variable-length records.

```cpp
struct trailing_u16
{
    uint8_t header;
    cpp_utils::serde::dynamic_array_until_eof<uint16_t> values;
};
std::vector<uint8_t> buffer { 9, 1, 0, 2, 0, 3, 0 };
auto s = cpp_utils::serde::deserialize<trailing_u16>(buffer);
// s.values.size() == 3
```

```cpp
// dynamic-size element case: each record carries its own length
struct record
{
    uint8_t len;
    cpp_utils::serde::dynamic_array_bytes<0, uint8_t> payload;
    std::size_t field_size(const decltype(payload)&) const { return len; }
};
struct records_until_eof
{
    cpp_utils::serde::dynamic_array_until_eof<record> records;
};
std::vector<uint8_t> buffer { 2, 0xAA, 0xBB, 1, 0xCC };
auto s = cpp_utils::serde::deserialize<records_until_eof>(buffer);
// s.records.size() == 2; s.records[0].payload == {0xAA, 0xBB}; s.records[1].payload == {0xCC}
```

On save, `dynamic_array_until_eof` is written exactly like any other `dynamic_array` — every element
currently in the container is written, with no length prefix or end marker — so it only makes sense
as the last field of a composite.

### `bounded_string<N>`

A `std::string` field (in member `.value`) read up to `N` bytes, stopping early at an embedded null
byte; written back zero-padded to exactly `N` bytes (truncated to `N` if longer). Declares
`wire_size = N` so `reflexion::field_size` reports `N` instead of `sizeof(std::string)`, and is a
`const_size_field`.

```cpp
struct with_name
{
    cpp_utils::serde::bounded_string<8> name;
    uint16_t value;
};
std::vector<uint8_t> buffer { 'a', 'b', 'c', 0, 0, 0, 0, 0, 5, 0 };
auto s = cpp_utils::serde::deserialize<with_name>(buffer);
// s.name.value == "abc", s.value == 5
```

### `scaled<wire_t, real_t, Num, Den = 1>`

A fixed-point wire field: an integer `wire_t` on the wire, exposed in memory as `real_t` (in
member `.value`), related by a compile-time scale factor `scale = Num / Den` (computed as `real_t`).

- load: `value = static_cast<real_t>(raw) * scale`
- save: `raw = llround(static_cast<double>(value) / static_cast<double>(scale))` (rounded, and
  computed in `double` regardless of `real_t`)

`wire_size = sizeof(wire_t)`, and it's a `const_size_field`.

```cpp
struct with_temperature
{
    cpp_utils::serde::scaled<int16_t, double, 1, 100> temperature; // raw LSB = 0.01 unit
};
// raw int16_t 1234 little-endian, scale 1/100 -> 12.34
std::vector<uint8_t> buffer { 0xD2, 0x04 };
auto s = cpp_utils::serde::deserialize<with_temperature>(buffer);
// s.temperature.value == Approx(12.34)
```

Round-trip (save direction):

```cpp
with_temperature original;
original.temperature.value = 12.34;
std::vector<uint8_t> sink;
cpp_utils::serde::serialize(original, sink, std::size_t { 0 });
// sink == {0xD2, 0x04}
```

### `unused<T>`

Wraps any other loadable field type `T` (a fundamental/enum type, or another `special_fields`
wrapper — not a plain user-defined composite, since only wrapper/fundamental/enum types have a
`load_field` overload to dispatch through). On load, the value is decoded into a temporary and
discarded — only the byte offset advances. On save, `save_field` requires `T` to be a
`const_size_field` (enforced by `static_assert`; a discarded dynamic-size field has no real value
left to measure a size from) and writes `sizeof(T)`/`T`'s wire size worth of zero bytes.

```cpp
struct with_reserved
{
    char a;
    cpp_utils::serde::unused<int32_t> reserved;
    char b;
};
std::vector<uint8_t> buffer { 'x', 0xDE, 0xAD, 0xBE, 0xEF, 'y' };
auto s = cpp_utils::serde::deserialize<with_reserved>(buffer);
// s.a == 'x', s.b == 'y' (the 4 middle bytes are consumed but not kept)

with_reserved value { 'x', {}, 'y' };
std::vector<uint8_t> sink;
cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
// sink == {'x', 0, 0, 0, 0, 'y'}
```

Loading a dynamic-size wrapped field (e.g. `unused<dynamic_array_bytes<0, uint16_t>>`) is supported
— only the save direction is restricted to constant-size `T`.

### `padding_bytes_t<size, value>`

Fixed-size skip/fill field with no associated data member of its own. On load, `offset` advances by
`size` bytes without inspecting them. On save, writes `size` bytes each equal to `value` (a
`uint8_t`).

```cpp
struct with_padding
{
    char a;
    cpp_utils::serde::padding_bytes_t<3, 0xFF> pad;
    char b;
};
with_padding value { 'x', {}, 'y' };
std::vector<uint8_t> sink;
cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
// sink == {'x', 0xFF, 0xFF, 0xFF, 'y'}
```
