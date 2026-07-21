# `types` module

Grab-bag of C++20 type-level utilities backing the rest of the library: concepts, compile-time
member/method/type detection idioms, smart-pointer helpers, integer helpers, a variant-visitor
builder, and a runtime-format-code-to-compile-time-type dispatcher. Each header is independently
usable — none of them depend on the rest of `cpp_utils` except where noted.

## Concepts

`#include <types/concepts.hpp>` — namespace `cpp_utils::types::concepts`

Constraint concepts used throughout the library (and directly usable in your own templates).

| Concept | Constrains |
|---|---|
| `pointer_to_contiguous_memory<T>` | `T` supports pointer arithmetic (`+`, `-`, `+=`, `-=`), dereference, and `operator[]` returning something convertible to `std::remove_pointer_t<T>`. |
| `fundamental_type<T>` | `std::is_fundamental_v<std::decay_t<T>>`. |
| `enum_type<T>` | `std::is_enum_v<std::decay_t<T>>`. |
| `container<T>` | `T` has `size()`, `max_size()`, `empty()`, `begin()`/`end()`, `cbegin()`/`cend()`. |
| `sequence_container<T>` | `container<T>` plus `t.front()` convertible to `T::value_type`. |
| `contiguous_sequence_container<T>` | `container<T>` and `T::iterator` is a `std::contiguous_iterator`. |
| `bounded_array<T>` | `std::is_bounded_array_v<T>` (e.g. `int[4]`, not `int*` or `int[]`). |
| `random_access_buffer<T>` | `T` has `read(dest, offset, size)`, `view(offset)`, `size()`, `is_valid()`. Satisfied by `cpp_utils::io::memory_mapped_file` and `cpp_utils::io::buffer_view`. |
| `contiguous_sized_range<T>` | `std::ranges::contiguous_range<T> && std::ranges::sized_range<T>` — `std::vector`, `std::array`, `std::span`, etc. Shared precondition for `threading::parallel_chunks_*` input ranges. |
| `chunk_for_each_callback<F, Input>` | `F` is invocable with a mutable `std::span` over `Input`'s element type — the per-chunk callback shape used by `threading::parallel_chunks_for_each`. |
| `chunk_transform_callback<F, Input, Output>` | `F` is invocable with a read-only input chunk span and an `Output` position — used by `threading::parallel_chunks_transform`. |
| `chunk_reduce_callback<F, Input, T>` | `F` reduces a read-only input chunk span down to a value convertible to `T` — used by `threading::parallel_chunks_reduce`. |
| `foldable_binary_op<BinaryOp, T>` | `BinaryOp(T, T)` returns something convertible to `T` (e.g. `std::plus<>{}`) — the combine step for `threading::parallel_chunks_reduce`. |

```cpp
#include <types/concepts.hpp>
#include <vector>

using namespace cpp_utils::types::concepts;

bool touches_raw_memory(pointer_to_contiguous_memory auto) { return true; }
bool touches_raw_memory(auto) { return false; }

double d;
touches_raw_memory(&d);                                // true
touches_raw_memory(std::vector { 1.0, 2.0, 3.0 });      // false — not a pointer

static_assert(contiguous_sized_range<std::vector<int>>);
static_assert(!contiguous_sized_range<int>);
```

## Detectors

`#include <types/detectors.hpp>` — namespace `cpp_utils::types::detectors`

Macros that generate compile-time "does this type have member/method/nested-type X" traits, plus
a couple of small standalone traits. Each generated trait comes as both a `struct` (usable as a
type, e.g. in `std::conjunction<...>`) and a `_v` bool variable template.

### `HAS_MEMBER(member)`

Declares `has_<member>_member_object<T>` / `has_<member>_member_object_v<T>`: true when `T::member`
is a (publicly accessible) member *object* — not a method.

```cpp
#include <types/detectors.hpp>

struct TestStruc { int valid; void method(); };

HAS_MEMBER(valid)
HAS_MEMBER(method)

static_assert(has_valid_member_object_v<TestStruc>);
static_assert(!has_method_member_object_v<TestStruc>);   // it's a method, not a data member
```

### `HAS_METHOD(name, method, ...)`

Declares `<name><T>` / `<name>_v<T>`: true when `t.method(args...)` is well-formed for the given
argument types (empty for a no-arg overload). Overload-aware — pass the argument types to pick a
specific overload.

```cpp
#include <types/detectors.hpp>

struct TestStruc
{
    void overload_method() {}
    void overload_method(int) {}
    void overload_method(double, int) {}
};

HAS_METHOD(has_overload_void_method, overload_method)
HAS_METHOD(has_overload_int_method, overload_method, int)
HAS_METHOD(has_overload_double_int_method, overload_method, double, int)

static_assert(has_overload_void_method_v<TestStruc>);
static_assert(has_overload_int_method_v<TestStruc>);
static_assert(has_overload_double_int_method_v<TestStruc>);
```

### `HAS_TYPE(type)`

Declares `has_<type>_type<T>` / `has_<type>_type_v<T>`: true when `T::type` names a type (nested
`using`/`typedef`, including reference-qualified ones).

```cpp
#include <types/detectors.hpp>

struct TestStruc { using valid = int; int value; };

HAS_TYPE(valid)
HAS_TYPE(value)   // "value" is a data member, not a type, so this will be false

static_assert(has_valid_type_v<TestStruc>);
static_assert(!has_value_type_v<TestStruc>);
```

### `IS_T(name, type)`

Declares `name<T>` / `name_v<T>`: true only for the exact type `type` (a `std::is_same_v`-style
check wrapped as a reusable trait).

```cpp
#include <types/detectors.hpp>

struct TestStruc {};
IS_T(is_TestStruc, TestStruc)

static_assert(is_TestStruc_v<TestStruc>);
static_assert(!is_TestStruc_v<int>);
```

### `IS_TEMPLATE_T(name, type)`

Same as `IS_T`, but matches any instantiation of a class template regardless of its template
arguments.

```cpp
#include <types/detectors.hpp>
#include <vector>

IS_TEMPLATE_T(is_std_vector, std::vector)

static_assert(is_std_vector_v<std::vector<int>>);
static_assert(!is_std_vector_v<int>);
```

### Other symbols

- `template <typename ref_type, typename... types> struct is_any_of` / `is_any_of_t` /
  `is_any_of_v` — true when `ref_type` is the same as any one of `types...`.

  ```cpp
  using namespace cpp_utils::types::detectors;
  static_assert(is_any_of_v<int, char, int, double>);
  static_assert(!is_any_of_v<int, char, double>);
  ```

- `is_qt_tree_item<T>` / `is_qt_tree_item_v<T>` — true when `T` exposes `takeChildren()`,
  `parent()`, and `addChild(nullptr)` (the shape of `QTreeWidgetItem`). Qt-oriented but has no Qt
  header dependency itself — it only requires those three calls to compile.
- `has_toStdString_method<T>` / `has_toStdString_method_v<T>` — generated via `HAS_METHOD`, true
  when `t.toStdString()` is callable (e.g. `QString`).

## dtype_dispatch

`#include <types/dtype_dispatch.hpp>` — namespace `cpp_utils::types`

```cpp
template <typename F>
auto dispatch_dtype(char format_code, F&& func);
```

Maps a single-character [Python buffer-protocol / `struct` module format
code](https://docs.python.org/3/library/struct.html#format-characters) (`'f'`, `'d'`, `'b'`,
`'B'`, `'h'`, `'H'`, `'i'`, `'I'`, `'l'`, `'L'`, `'q'`, `'Q'`) to the matching C++ type, calling
`func(std::type_identity<T>{})` and returning whatever `func` returns. Every branch of `func` must
return the same type (it's a `switch`, not a variant dispatch). Throws `std::invalid_argument` for
an unrecognized code. Meant for bridging a runtime dtype tag (e.g. from a numpy array or Python
buffer-protocol object) into compile-time-typed code without hand-writing the switch yourself.

```cpp
#include <types/dtype_dispatch.hpp>

using namespace cpp_utils::types;

auto size_of_code = [](char code)
{
    return dispatch_dtype(
        code, [](auto type_tag) { return sizeof(typename decltype(type_tag)::type); });
};

size_of_code('f');   // == sizeof(float)
size_of_code('q');   // == sizeof(long long)

dispatch_dtype('z', [](auto) { return 0; });   // throws std::invalid_argument
```

## Integers

`#include <types/integers.hpp>` — namespace `cpp_utils::types`

```cpp
template <std::size_t s> using uint_t = /* unsigned integer type of size s bytes */;
template <typename T> using uint_of_the_same_size_t = uint_t<sizeof(std::decay_t<T>)>;
```

`uint_t<s>` maps a byte size (1, 2, 4, or 8 — any other value is a compile error, no specialization
exists) to the matching fixed-width unsigned type (`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`).
`uint_of_the_same_size_t<T>` is the convenience form that derives the size from an existing type —
handy for bit-reinterpreting a float/double as an unsigned integer of the same width without
hard-coding which one.

```cpp
#include <types/integers.hpp>

using cpp_utils::types::uint_t;
using cpp_utils::types::uint_of_the_same_size_t;

static_assert(std::is_same_v<uint_t<4>, uint32_t>);
static_assert(std::is_same_v<uint_of_the_same_size_t<double>, uint64_t>);
static_assert(std::is_same_v<uint_of_the_same_size_t<float>, uint32_t>);
```

## Pointers

`#include <types/pointers.hpp>` — namespace `cpp_utils::types::pointers`

| Symbol | Description |
|---|---|
| `is_dereferencable<T>` / `is_dereferencable_v<T>` | True when `*std::declval<T>()` is well-formed. |
| `is_std_shared_ptr<T>` / `_v` | True for `std::shared_ptr<U>`, any `U`. |
| `is_std_unique_ptr<T>` / `_v` | True for `std::unique_ptr<U>`, any `U`. |
| `is_std_weak_ptr<T>` / `_v` | True for `std::weak_ptr<U>`, any `U`. |
| `is_std_smart_ptr<T>` / `_v` | True if `T` is any of the three above. |
| `is_smart_ptr<T>` / `_v` | Currently equivalent to `is_std_smart_ptr` — the extra indirection is the extension point for non-`std` smart pointer types. |
| `to_value(T&& item)` | Dereferences: raw pointers and smart pointers (per `is_smart_ptr_v`) are dereferenced (`*item` / `*item.get()`), returning a value copy; anything else is passed through unchanged. |
| `to_value_ref(T&& item)` | Same dereferencing logic as `to_value`, but returns a reference (`auto&`) instead of a copy. |

```cpp
#include <types/pointers.hpp>
#include <memory>

using namespace cpp_utils::types::pointers;

static_assert(is_std_unique_ptr<std::unique_ptr<double>>::value);
static_assert(is_smart_ptr<std::shared_ptr<double>>::value);
static_assert(!is_smart_ptr<double*>::value);   // raw pointers aren't "smart" pointers

double d = 3.14;
to_value(&d);                              // double, copy of 3.14
to_value(std::make_unique<double>(2.0));   // double, copy of 2.0
to_value_ref(d);                           // double&, aliases d
```

## Strings

`#include <types/strings.hpp>` — namespace `cpp_utils::types::strings`

```cpp
IS_T(is_std_string, std::string)   // generates is_std_string<T> / is_std_string_v<T>

template <typename str_t, typename T>
std::enable_if_t<is_std_string_v<str_t>, std::string> to_string(const T& object);
```

`is_std_string<T>` / `is_std_string_v<T>` is a detector generated via `detectors.hpp`'s `IS_T`
macro, true only for `T = std::string`. `to_string<str_t>(object)` is SFINAE-gated on
`is_std_string_v<str_t>` and forwards to `std::to_string(object)` — `str_t` exists purely to
constrain the overload (it plays no role in computing the result); pass any type with a matching
`std::to_string` overload as `object`.

This header also adds two overloads to namespace `std` — `std::to_string(const std::string&)` and
`std::to_string(std::string&&)`, both identity operations — so `std::to_string` can be called
uniformly on a value that might already be a `std::string`.

```cpp
#include <types/strings.hpp>

using namespace cpp_utils::types::strings;

to_string<std::string>(42);        // "42", via std::to_string(int)
std::to_string(std::string("x"));  // "x", identity overload added by this header
```

## Visitor

`#include <types/visitor.hpp>` — namespace `cpp_utils::types`

```cpp
template <typename... Ts>
struct Visitor : Ts... { using Ts::operator()...; };

template <typename... Ts>
Visitor(Ts...) -> Visitor<Ts...>;
```

The classic "overload set from lambdas" idiom: `Visitor` inherits from each callable passed in and
pulls all their `operator()`s into one overload set, so a `Visitor{lambda1, lambda2, ...}` can be
passed anywhere a single callable is expected (most commonly `std::visit`) and dispatches to
whichever lambda matches the argument type. CTAD (the deduction guide) means you never spell out
`Ts...` yourself — just list the lambdas.

```cpp
#include <types/visitor.hpp>
#include <variant>
#include <string>

using namespace cpp_utils::types;

std::variant<int, std::string> v = 42;

auto describe = [](const auto& value)
{
    return std::visit(Visitor { [](int i) { return std::string("int:") + std::to_string(i); },
                           [](const std::string& s) { return std::string("string:") + s; } },
        value);
};

describe(v);   // "int:42"
v = std::string("hello");
describe(v);   // "string:hello"
```
