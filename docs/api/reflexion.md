# reflexion

`reflexion` counts and walks the members of a C++20 aggregate at compile time, without macros,
codegen, or per-type boilerplate. It is the mechanism the rest of `cpp_utils` (most notably
`serde`) uses to iterate a struct's fields generically, but it is self-contained and usable on its
own by anything that needs "for each field of this aggregate, do X" at compile time.

```cpp
#include <reflexion/reflection.hpp>
```

All symbols below live in namespace `cpp_utils::reflexion`, except the two `SPLIT_FIELDS*` macros,
which are global (no namespace, C-preprocessor macros).

## count_members

```cpp
template <typename T>
inline constexpr std::size_t count_members;
```

The number of members `T` destructures into with a structured binding. Computed at compile time
via a `consteval` binary search: it probes the largest `N` for which `T` can be aggregate-
initialized from `N` implicitly-convertible-to-anything arguments.

A member that is itself a fixed-size C array counts as **one** member (matching structured-binding
semantics — the array binds as a single element, it does not get flattened).

```cpp
struct five_members { int a, b, c, d, e; };
struct with_c_array_member { int a; char buf[8]; int b; };

static_assert(cpp_utils::reflexion::count_members<five_members> == 5);
static_assert(cpp_utils::reflexion::count_members<with_c_array_member> == 3); // a, buf, b
```

**Hard ceiling: 31 members.** `count_members` (and therefore everything built on it, including
`SPLIT_FIELDS`) does not support aggregates with more than 31 members. Instantiating it on such a
type is a compile error raised by a `static_assert` inside `count_members` itself — not merely a
later failure in `SPLIT_FIELDS` — so the error surfaces at the point of first use.

## can_split_v

```cpp
template <typename T>
consteval auto can_split();

template <typename T>
inline constexpr bool can_split_v = can_split<T>();
```

Whether `T` should be treated as a composite to recurse into (`true`) or as one atomic field
(`false`). `can_split_v<T>` is `false` for fundamental types, enums, and any type opting out via
the `do_not_split_t` tag (see below); it is `true` for everything else — i.e. ordinary aggregates.

```cpp
struct point { int x, y; };
enum class color { red, green, blue };

static_assert(cpp_utils::reflexion::can_split_v<point>);
static_assert(!cpp_utils::reflexion::can_split_v<int>);
static_assert(!cpp_utils::reflexion::can_split_v<color>);
```

## do_not_split_t

```cpp
struct do_not_split_t {};
```

Tag type used to opt a type out of field-splitting, even though it is itself an aggregate. Give
the type a nested `using do_not_split = cpp_utils::reflexion::do_not_split_t;` and `can_split_v`
(and everything that checks it, e.g. `composite_size`'s per-field recursion) will treat it as one
opaque field instead of recursing into its members.

This is the general convention `cpp_utils` uses for attaching cross-cutting metadata to a type: a
nested type alias checked with `requires { typename T::alias_name; }`, rather than a specialized
trait. `dyn_size_field_tag_t` below follows the same pattern.

```cpp
struct raw_blob
{
    using do_not_split = cpp_utils::reflexion::do_not_split_t;
    unsigned char bytes[16];
};

static_assert(!cpp_utils::reflexion::can_split_v<raw_blob>);
```

## is_dyn_size_field_v

```cpp
template <typename T>
consteval auto is_dyn_size_field();

template <typename T>
inline constexpr bool is_dyn_size_field_v = is_dyn_size_field<std::decay_t<T>>();
```

Whether `T` (after decaying references/cv-qualifiers) is tagged as not having a compile-time-known
size — i.e. carries the `dyn_size_field_tag_t` tag described below. `composite_have_const_size`
and `composite_size` use this to detect fields whose size can only be known at runtime.

## dyn_size_field_tag_t

```cpp
struct dyn_size_field_tag_t {};
```

Tag type marking a field type as dynamically sized. Give the type a nested
`using dyn_size_field_tag = cpp_utils::reflexion::dyn_size_field_tag_t;` to make
`is_dyn_size_field_v<T>` true for it. A type tagged this way makes any composite that (directly or
through a nested composite) contains it report `false` from `composite_have_const_size`.

```cpp
struct variable_length_blob
{
    using do_not_split = cpp_utils::reflexion::do_not_split_t;
    using dyn_size_field_tag = cpp_utils::reflexion::dyn_size_field_tag_t;
    std::vector<unsigned char> bytes;
};

static_assert(cpp_utils::reflexion::is_dyn_size_field_v<variable_length_blob>);
```

## composite_have_const_size

```cpp
template <typename T>
consteval std::size_t composite_have_const_size();
```

Whether every field of `T` (recursively, through nested composites) has a compile-time-known size.
The return type is `std::size_t` for implementation reasons, but the value is always `0` or `1` —
treat it as a boolean (it's used directly as a `static_assert` condition inside `composite_size`,
see below). Fundamental types and enums are always constant-size. A type directly tagged with
`dyn_size_field_tag_t` is never constant-size.

```cpp
struct fixed_record { int a; double b; };

static_assert(cpp_utils::reflexion::composite_have_const_size<fixed_record>());

struct record_with_dyn_field
{
    int a;
    variable_length_blob payload; // from the dyn_size_field_tag_t example above
};

static_assert(!cpp_utils::reflexion::composite_have_const_size<record_with_dyn_field>());
```

## composite_size

```cpp
template <typename composite_t>
consteval std::size_t composite_size();
```

The compile-time byte size of `composite_t`, computed by recursively summing per-field sizes
(nested splittable composites recurse via `composite_size` again; atomic fields go through
`field_size`, see below). Fails to compile (`static_assert`) if `composite_have_const_size` is
`false` for the type — there is no such thing as `composite_size` of a dynamically-sized type.

```cpp
struct point { int x, y; };
struct nested_fixed { point p; int z; };

static_assert(cpp_utils::reflexion::composite_size<nested_fixed>()
    == sizeof(point) + sizeof(int));
```

A fixed-size C array member is sized correctly too — `composite_size` reports its full
`sizeof` (`char buf[8]` contributes 8 bytes, matching `count_members` treating it as one field,
see above). Wire-format structs still generally prefer a dedicated wrapper type over a bare C
array (e.g. `serde::static_array<T, N>`) for the richer API — indexing, iteration, `wire_size` —
those wrappers provide, not because a bare array is mis-sized.

## field_size

```cpp
template <typename field_t>
[[nodiscard]] consteval std::size_t field_size();
```

The size, in bytes, of a single non-composite (or `do_not_split`-tagged) field type. In order:
uses `field_t::wire_size` if the type declares one (the mechanism wrapper types like `serde`'s
`bounded_string<N>` use to report a size other than `sizeof`); otherwise recurses via
`composite_size<field_t>()` if `can_split_v<field_t>` is true; otherwise falls back to
`sizeof(field_t)`. Asserts at compile time that `field_t` is not `dyn_size_field_tag_t`-tagged.

```cpp
static_assert(cpp_utils::reflexion::field_size<int>() == sizeof(int));
static_assert(cpp_utils::reflexion::field_size<point>() == sizeof(point));
```

## field_have_const_size

```cpp
template <typename field_t>
[[nodiscard]] consteval bool field_have_const_size();
```

The atomic-field counterpart to `composite_have_const_size`: `true` unless `field_t` is
`dyn_size_field_tag_t`-tagged.

```cpp
static_assert(cpp_utils::reflexion::field_have_const_size<int>());
```

## SPLIT_FIELDS_FW_DECL / SPLIT_FIELDS

```cpp
#define SPLIT_FIELDS_FW_DECL(return_type, name, const_struct)
#define SPLIT_FIELDS(return_type, name, function, const_struct)
```

The macro pair `composite_size`/`composite_have_const_size` are themselves built with. Given a
count from `count_members`, `SPLIT_FIELDS` generates a function `name` that structured-binds its
`structure` argument into `_0.._N` and forwards them positionally, as trailing arguments, to
`function` — one `if constexpr` branch per member count from 0 to 31. This is how the rest of the
library turns "count the members" into "do something with each member" without writing a manual
switch per struct.

- `return_type` — the return type of the generated function (and of `function`).
- `name` — the name of the generated function, callable as `name(structure, args...)`.
- `function` — the callback invoked as `function(structure, args..., field_0, ..., field_N)`. It
  must itself be overloaded/recursive over its trailing pack, since it receives a different number
  of field arguments depending on `T`.
- `const_struct` — either `const` or empty, controlling whether `structure` is taken by
  `const T&` or plain `T&` (`composite_size` and `composite_have_const_size` both use `const`,
  since they only read fields).

`SPLIT_FIELDS_FW_DECL` emits just the forward declaration (same signature, no body) — use it when
`function` and `name` need to see each other's declarations before `SPLIT_FIELDS` defines `name`.

A minimal, complete usage — a function that counts fields the same way `count_members` does, but
by actually visiting them:

```cpp
#include <reflexion/reflection.hpp>

SPLIT_FIELDS_FW_DECL(constexpr int, count_fields, const)

constexpr int accumulate_field_count(const auto&)
{
    return 0;
}

template <typename record_t, typename T, typename... Ts>
constexpr int accumulate_field_count(const record_t& s, T&&, Ts&&... rest)
{
    return 1 + accumulate_field_count(s, std::forward<Ts>(rest)...);
}

SPLIT_FIELDS(constexpr int, count_fields, accumulate_field_count, const)

struct five_members { int a, b, c, d, e; };
static_assert(count_fields(five_members {}) == 5);
```

`fields_size`/`composite_size` and `fields_have_const_size`/`composite_have_const_size` in this
header are the same pattern applied for real: the `function` callback there is overloaded over
0, 1, and N remaining fields, accumulating a `std::size_t` sum or a boolean AND respectively.
