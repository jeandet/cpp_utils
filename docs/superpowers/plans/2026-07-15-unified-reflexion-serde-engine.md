# Unified reflexion + serde engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a unified, general reflection + binary serialize/deserialize engine in `cpp_utils`, fixing the three correctness/complexity bugs found by comparing it against CDFpp's parallel copy, and extending it with a serialize (write) path that doesn't exist today.

**Architecture:** Extend `reflexion/reflection.hpp`'s member-counting core with a binary-search algorithm (replacing the current O(N) linear scan with a dead code path). Extend `serde/` with `std::span`-based buffers, an optional threaded `context` parameter, three new field-wrapper types (`bounded_string`, `dynamic_array_bytes`, `unused`), and a `serialize`/`save_field` path that mirrors the existing `deserialize`/`load_field` path field-for-field. Add `runtime_size` for computing a value's actual wire length.

**Tech Stack:** C++20 (concepts, `consteval`, `std::span`, `<bit>`), Meson + Ninja, Catch2 v3.

## Global Constraints

- **C++20 only** — no `std::byteswap`, `if consteval`, deducing-this, `std::expected`, or any other C++23-only construct anywhere in new/modified code. Driven by CDFpp's need to build wheels via `cibuildwheel` across manylinux/macOS/Windows, where C++23 support is not uniformly reliable.
- **31-member cap on `SPLIT_FIELDS` stays unchanged** — types with more members must hard-error at compile time, never silently truncate.
- **No buffer bounds-checking** — callers are assumed to have already validated buffer size before calling into the engine, matching current behavior. Do not add runtime range checks as part of this plan.
- Every new/modified header keeps the existing MIT license comment block used throughout `cpp_utils` (copy verbatim from any existing header in the same directory — do not paraphrase it).
- Formatting follows `.clang-format` (WebKit-based, Allman braces, 4-space indent, 100 col). Run `clang-format -i <file>` on every file you touch before committing if `clang-format` is available; if not installed, match brace/indent style by eye against the surrounding code.
- Build/test loop used throughout this plan: `meson setup build` (first time only), `ninja -C build`, `meson test -C build <TestName>` for a single test, `meson test -C build` for the full suite.

---

### Task 1: Reflexion — binary-search `count_members`

**Files:**
- Modify: `include/cpp_utils/reflexion/reflection.hpp:35-57` (the `details` namespace containing `anything` and `MemberCounter`)
- Create: `tests/reflexion/test_count_members.cpp`
- Modify: `tests/meson.build` (add negative-compile check + register new test)

**Interfaces:**
- Produces: `cpp_utils::reflexion::count_members<T>` (unchanged public name/signature — a `constexpr std::size_t` variable template). All downstream consumers (`serde/deserialization.hpp`, `serde/special_fields.hpp`) keep working unmodified.

Background: the current `details::MemberCounter` has a dead code path — tracing it, the branch that would grow the `c0` parameter pack is unreachable, because with `c0` empty the two `requires`-expressions it distinguishes between are byte-for-byte identical. It always grows `A0` one member at a time: a linear scan. It also has no cap, so `count_members<T>` alone (without going through `SPLIT_FIELDS`) never rejects an oversized type — only `SPLIT_FIELDS`'s separate `static_assert(count <= 31)` catches that, and only when something actually calls `SPLIT_FIELDS` on the type. This task replaces it with a real binary search that also rejects oversized types from `count_members` itself.

- [ ] **Step 1: Write the reproducer (negative-compile check) proving today's code does NOT reject >31-member types via `count_members` alone**

Add to `tests/meson.build`, near the top, before the `tests = [...]` array:

```meson
too_many_members_fragment = '''
#include "reflexion/reflection.hpp"
struct thirty_two_members
{
    int a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,
        a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31;
};
constexpr auto n = cpp_utils::reflexion::count_members<thirty_two_members>;
int main() { return static_cast<int>(n); }
'''
assert(not cpp_compiler.compiles(too_many_members_fragment, include_directories : cpp_utils_inc,
    name : 'count_members must reject types with more than 31 members'),
    'cpp_utils::reflexion::count_members should fail to compile for >31-member types, but it compiled')
```

Note: `cpp_compiler` and `cpp_utils_inc` are already defined in the root `meson.build` and are in scope here because `tests/meson.build` is `subdir()`'d from it.

- [ ] **Step 2: Run it to confirm it fails against today's code**

Run: `meson setup build --reconfigure` (or `meson setup build` if `build/` doesn't exist yet)
Expected: meson configure **fails** with the message `cpp_utils::reflexion::count_members should fail to compile for >31-member types, but it compiled` — because today's `MemberCounter` has no cap and happily returns 32.

- [ ] **Step 3: Write the exact-count static_assert tests (will compile fine both before and after — these pin down correctness, the negative-compile check above is what's currently broken)**

Create `tests/reflexion/test_count_members.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <reflexion/reflection.hpp>

struct one_member
{
    int a;
};

struct five_members
{
    int a, b, c, d, e;
};

struct thirty_one_members
{
    int a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,
        a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30;
};

struct with_c_array_member
{
    int a;
    char buf[8];
    int b;
};

static_assert(cpp_utils::reflexion::count_members<one_member> == 1);
static_assert(cpp_utils::reflexion::count_members<five_members> == 5);
static_assert(cpp_utils::reflexion::count_members<thirty_one_members> == 31);
static_assert(cpp_utils::reflexion::count_members<with_c_array_member> == 3);

TEST_CASE("count_members compile-time checks", "[reflexion]")
{
    REQUIRE(cpp_utils::reflexion::count_members<one_member> == 1);
    REQUIRE(cpp_utils::reflexion::count_members<with_c_array_member> == 3);
}
```

- [ ] **Step 4: Register the new test in `tests/meson.build`**

Add to the `tests` list array (alongside the existing entries):

```meson
    {
        'name':'TestCountMembers',
        'sources': ['reflexion/test_count_members.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

- [ ] **Step 5: Implement the binary-search counting engine**

Replace `include/cpp_utils/reflexion/reflection.hpp:35-57` (the whole `details` namespace block, from `namespace details` down to its closing `}`) with:

```cpp
namespace details
{
struct anything
{
    template <class T>
        requires(!std::is_bounded_array_v<std::decay_t<T>>)
    operator T() const;
};

template <typename T, std::size_t N>
consteval bool can_construct_with_n()
{
    return []<std::size_t... Is>(std::index_sequence<Is...>)
    {
        return requires { T { (void(Is), anything {})... }; };
    }(std::make_index_sequence<N> {});
}

template <typename T, std::size_t Lo, std::size_t Hi>
consteval std::size_t binary_search_member_count()
{
    if constexpr (Lo == Hi)
        return Lo;
    else
    {
        constexpr std::size_t mid = Lo + (Hi - Lo + 1) / 2;
        if constexpr (can_construct_with_n<T, mid>())
            return binary_search_member_count<T, mid, Hi>();
        else
            return binary_search_member_count<T, Lo, mid - 1>();
    }
}

template <typename T, std::size_t Cap = 31>
consteval std::size_t MemberCounter()
{
    static_assert(can_construct_with_n<T, Cap + 1>() == false,
        "cpp_utils::reflexion: type has more than 31 members, which is not supported by "
        "SPLIT_FIELDS");
    return binary_search_member_count<T, 0, Cap>();
}

}
```

Nothing else in the file needs to change: `count_members`'s definition (`inline constexpr std::size_t count_members = details::MemberCounter<T>();`) calls `MemberCounter<T>()` with no arguments, which matches the new signature exactly.

- [ ] **Step 6: Run the full reflexion + serde test suite**

Run: `meson setup build --reconfigure && ninja -C build && meson test -C build`
Expected: configure succeeds (the negative-compile check now correctly reports "did not compile" and passes), all existing tests still pass (`Test-TreePrint`, `Test-TestDeserialization`, etc. — this task changes internals only, `count_members` returns identical values for every type already in the test suite), and the new `Test-TestCountMembers` passes.

- [ ] **Step 7: Commit**

```bash
git add include/cpp_utils/reflexion/reflection.hpp tests/reflexion/test_count_members.cpp tests/meson.build
git commit -m "reflexion: replace O(N) MemberCounter with binary search, fix silent >31-member truncation"
```

---

### Task 2: Endianness — detect host byte order via `<bit>`

**Files:**
- Modify: `include/cpp_utils/endianness/endianness.hpp:24-35`
- Modify: `config.h.in`
- Modify: `include/cpp_utils/meson.build`

**Interfaces:**
- Produces: `cpp_utils::endianness::host_is_big_endian`, `host_is_little_endian` (unchanged names/values, now `constexpr` instead of runtime `const`).

This is a behavior-preserving refactor (same public constants, same values on every real platform), so there is no new failing test to write — the existing endianness and serde test suites are the regression check. Run them before and after to confirm zero behavior change.

- [ ] **Step 1: Run the existing suite to record the baseline**

Run: `meson test -C build Test-TestEndianness Test-TestDeserialization`
Expected: both PASS (this is the baseline you'll compare against after the change).

- [ ] **Step 2: Replace the macro-based detection**

In `include/cpp_utils/endianness/endianness.hpp`, replace lines 24-35:

```cpp
#include "config.h"
#ifdef CPP_UTILS_BIG_ENDIAN
inline const bool host_is_big_endian = true;
inline const bool host_is_little_endian = false;
#else
#ifdef CPP_UTILS_LITTLE_ENDIAN
inline const bool host_is_big_endian = false;
inline const bool host_is_little_endian = true;
#else
#error "Can't find if platform is either big or little endian"
#endif
#endif
```

with:

```cpp
#include <bit>

inline constexpr bool host_is_big_endian = std::endian::native == std::endian::big;
inline constexpr bool host_is_little_endian = std::endian::native == std::endian::little;

static_assert(host_is_big_endian != host_is_little_endian,
    "cpp_utils::endianness: mixed-endian hosts are not supported");
```

(The `#include "config.h"` is dropped entirely — nothing else in this file uses it. `CPP_UTILS_VERSION` and `CPP_UTILS_CONCEPTS_SUPPORTED` are still available where needed via `types/concepts.hpp`, which includes `config.h` directly.)

- [ ] **Step 3: Remove the now-dead config generation**

In `config.h.in`, remove these two lines (keep `CPP_UTILS_VERSION` and `CPP_UTILS_CONCEPTS_SUPPORTED`):

```
#mesondefine CPP_UTILS_BIG_ENDIAN
#mesondefine CPP_UTILS_LITTLE_ENDIAN
```

In `include/cpp_utils/meson.build`, remove this block:

```meson
if(target_machine.endian() == 'big')
    conf_data.set('CPP_UTILS_BIG_ENDIAN', true)
else
    conf_data.set('CPP_UTILS_LITTLE_ENDIAN', true)
endif
```

- [ ] **Step 4: Re-run the suite and confirm identical results**

Run: `meson setup build --reconfigure && ninja -C build && meson test -C build Test-TestEndianness Test-TestDeserialization`
Expected: both PASS, same as the Step 1 baseline.

- [ ] **Step 5: Commit**

```bash
git add include/cpp_utils/endianness/endianness.hpp config.h.in include/cpp_utils/meson.build
git commit -m "endianness: detect host byte order via <bit> instead of generated config.h macros"
```

---

### Task 3: Deserialize — `std::span` buffer view + threaded `context` parameter

**Files:**
- Create: `include/cpp_utils/serde/context.hpp`
- Modify: `include/cpp_utils/serde/deserialization.hpp` (full-file rewrite — nearly every signature gains a `context` parameter)
- Modify: `tests/serde/test_deserialization.cpp` (update call sites)
- Modify: root `meson.build` (add `context.hpp` to `headers`)

**Interfaces:**
- Produces (in `cpp_utils::serde`): `struct no_context {}` (default context tag); `deserialize<composite_t>(input, offset = 0, context = no_context{})`; the `SPLIT_FIELDS`-generated `deserialize(structure, parsing_context, offset, context)`; `load_field(parent, parsing_context, offset, context, field)` overload set. Everything downstream (Tasks 4-8) builds on this exact 5-parameter `load_field` shape and on `details::resolve_field_size` from `context.hpp`.
- Consumes: `reflexion::count_members`, `reflexion::can_split_v` (Task 1, unchanged names).

Two changes land together here because they're both signature-level refactors to the same functions and doing them separately would mean rewriting this file twice: (1) replace the duck-typed `_pointer_to_memory` (which **throws `std::runtime_error` at runtime** for an unsupported input type) with `std::span`-based conversion, which turns that into a compile error; (2) thread an optional `context` parameter through every call so a field's size can depend on data outside its immediate parent (needed by Task 6's `dynamic_array_bytes`).

Note the `std::span` change is a real, intentional API change: today's `deserialize<two_chars>(buffer.c_str())` passes a bare `const char*` with no size — `std::span` cannot be constructed from a bare pointer alone (no deduction guide for it), so that call form no longer compiles. This is the point: an "I don't know how big this buffer is" call is exactly the unsafe pattern we want to make impossible. Callers now pass the container itself (`deserialize<two_chars>(buffer)` where `buffer` is a `std::string`).

- [ ] **Step 1: Write the failing test (span-based call form doesn't exist yet)**

Modify `tests/serde/test_deserialization.cpp`: change every `deserialize<T>(buffer.c_str())` call to `deserialize<T>(buffer)`, e.g. the first test case becomes:

```cpp
    GIVEN("A simple struct with two chars")
    {
        struct two_chars
        {
            char a;
            char b;
        };
        std::string buffer { "cd" };
        WHEN("Deserializing it")
        {
            auto s = cpp_utils::serde::deserialize<two_chars>(buffer);
            THEN("It should have the right values")
            {
                REQUIRE(s.a == 'c');
                REQUIRE(s.b == 'd');
            }
        }
    }
```

Apply the same `buffer.c_str()` → `buffer` change to the "nested structs" test case (the other test cases already pass `std::array<uint8_t, N>` directly, not `.c_str()`, so they're unaffected).

- [ ] **Step 2: Run it to verify it fails**

Run: `ninja -C build`
Expected: **compile failure** — `deserialize<two_chars>(buffer)` doesn't match the current `deserialize(auto&& parsing_context, std::size_t offset = 0)` overload's expectations the way the implementation will require after Step 4 (actually: right now, before any implementation change, `deserialize<two_chars>(buffer)` where `buffer` is `std::string` already compiles today, since the current code accepts anything and duck-types it — so this step's real purpose is confirming the OLD `.c_str()` call form still compiles today too, both are accepted). Skip strict "must fail" here; proceed to Step 3 first, then Step 4, then verify in Step 5 that only the span-based form works and a bare-pointer form is rejected.

- [ ] **Step 3: Create the shared context header**

Create `include/cpp_utils/serde/context.hpp` (copy the MIT license header block from `include/cpp_utils/serde/special_fields.hpp:1-25` verbatim, then):

```cpp
#pragma once
#include "special_fields.hpp"

namespace cpp_utils::serde
{

struct no_context
{
};

namespace details
{
    template <typename parent_t, typename field_t, typename context_t>
    constexpr auto resolve_field_size(
        const parent_t& parent, const field_t& field, const context_t& context)
    {
        if constexpr (requires { parent.field_size(field, context); })
            return parent.field_size(field, context);
        else
            return parent.field_size(field);
    }
}

}
```

- [ ] **Step 4: Rewrite `deserialization.hpp`**

Replace the full contents of `include/cpp_utils/serde/deserialization.hpp` (keep the existing MIT license header block, lines 1-26) with:

```cpp
#pragma once
#include "../endianness/endianness.hpp"
#include "../reflexion/reflection.hpp"
#include "../types/concepts.hpp"
#include "context.hpp"
#include "special_fields.hpp"
#include <cstddef>
#include <span>

namespace cpp_utils::serde
{

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, deserialize, );

template <typename input_t, typename field_t, typename parent_composite_t, typename context_t>
std::size_t load_field(const parent_composite_t& parent_composite, input_t& parsing_context,
    std::size_t offset, const context_t& context, field_t& field);


namespace details
{

    template <typename input_t>
    std::span<const std::byte> to_byte_view(const input_t& input)
    {
        return std::as_bytes(std::span { input });
    }

    std::size_t _load_value_from_memory(std::span<const std::byte> input, std::size_t offset,
        types::concepts::fundamental_type auto& dest, const auto& parent_composite)
    {
        using T = std::decay_t<decltype(dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        dest = endianness::decode<endianness_t<parent_composite_t>, T>(
            reinterpret_cast<const char*>(input.data()) + offset);
        return offset + sizeof(T);
    }

    std::size_t _load_values_from_memory(std::span<const std::byte> input, std::size_t offset,
        types::concepts::fundamental_type auto* dest, std::size_t count,
        const auto& parent_composite)
    {
        using T = std::decay_t<decltype(*dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        endianness::decode_v<endianness_t<parent_composite_t>, T>(
            reinterpret_cast<const char*>(input.data()) + offset, count * sizeof(T), dest);
        return offset + sizeof(T) * count;
    }
}

std::size_t load_value(const auto& input, std::size_t offset,
    types::concepts::fundamental_type auto& dest, const auto& parent_composite)
{
    return details::_load_value_from_memory(
        details::to_byte_view(input), offset, dest, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, types::concepts::fundamental_type auto& field)
{
    (void)context;
    return load_value(parsing_context, offset, field, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, types::concepts::enum_type auto& field)
{
    using underlying_t = std::underlying_type_t<std::decay_t<decltype(field)>>;
    underlying_t& underlying_field = reinterpret_cast<underlying_t&>(field);
    return load_field(parent_composite, parsing_context, offset, context, underlying_field);
}

template <typename field_t>
concept dyn_array_field = dynamic_array_field<field_t> && !dynamic_array_until_eof_field<field_t>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dyn_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = details::resolve_field_size(parent_composite, array_field, context);
    if (count > 0)
    {
        array_field.resize(count);
        if constexpr (std::is_compound_v<field_t>)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                offset = deserialize(array_field[i],
                    std::forward<decltype(parsing_context)>(parsing_context), offset, context);
            }
            return offset;
        }
        else
        {
            return details::_load_values_from_memory(details::to_byte_view(parsing_context),
                offset, array_field.data(), count, parent_composite);
        }
    }
    return offset;
}

template <typename field_t>
concept dy_arr_until_eof_of_const_size = dynamic_array_until_eof_field<std::decay_t<field_t>>
    && const_size_field<typename std::decay_t<field_t>::value_type>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dy_arr_until_eof_of_const_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    constexpr auto field_size = reflexion::field_size<field_t>();
    const auto count = (parsing_context.size() - offset) / field_size;

    if (count > 0)
    {
        array_field.resize(count);
        if constexpr (reflexion::can_split_v<field_t>)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                offset = deserialize(array_field[i],
                    std::forward<decltype(parsing_context)>(parsing_context), offset, context);
            }
            return offset;
        }
        else
        {
            return details::_load_values_from_memory(details::to_byte_view(parsing_context),
                offset, array_field.data(), count, parent_composite);
        }
    }
    return offset;
}

template <typename field_t>
concept dy_arr_until_eof_of_dyn_size
    = dynamic_array_until_eof_field<field_t> && !const_size_field<typename field_t::value_type>;


constexpr inline std::size_t load_field(const auto&, auto& parsing_context, std::size_t offset,
    const auto& context, dy_arr_until_eof_of_dyn_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;

    static_assert(std::is_compound_v<field_t>, "Only compound types are supported");
    while (offset < std::size(parsing_context))
    {
        array_field.emplace_back(field_t {});
        offset = deserialize(
            array_field.back(), std::forward<decltype(parsing_context)>(parsing_context), offset,
            context);
    }
    return offset;
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, static_array_field auto& array_field)
{
    (void)context;
    constexpr auto count = std::decay_t<decltype(array_field)>::array_size;
    return details::_load_values_from_memory(details::to_byte_view(parsing_context), offset,
        array_field.data(), count, parent_composite);
}

template <typename composite_t, typename parsing_context_t, typename context_t, typename T>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return deserialize(field, parsing_context, offset, context);
    else
        return load_field(r, parsing_context, offset, context, std::forward<T>(field));
}

template <typename composite_t, typename parsing_context_t, typename context_t, typename T,
    typename... Ts>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field, Ts&&... fields)
{
    offset = load_fields(r, parsing_context, offset, context, std::forward<T>(field));
    return load_fields(r, parsing_context, offset, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, deserialize, load_fields, );

template <typename composite_t>
constexpr composite_t deserialize(auto&& parsing_context, [[maybe_unused]] std::size_t offset = 0,
    const auto& context = no_context {})
{
    composite_t r;
    deserialize(r, std::forward<decltype(parsing_context)>(parsing_context), offset, context);
    return r;
}

}
```

Note what changed vs. today's file: `_pointer_to_memory` is gone, replaced by `details::to_byte_view` (span-based); every `load_field`/`load_fields` overload gained a `const auto& context` parameter (positioned right after `offset`, before the field); the `dyn_array_field` overload now resolves its count via `details::resolve_field_size(parent_composite, array_field, context)` instead of calling `parent_composite.field_size(array_field)` directly (this is what makes Task 6's context-dependent sizing possible without touching this file again); `SPLIT_FIELDS_FW_DECL`'s matching forward-declared `load_field` template gained a `context_t` parameter too.

- [ ] **Step 5: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the updated `Test-TestDeserialization`. As a quick manual sanity check that the span change actually landed, confirm `echo '#include <cpp_utils/serde/deserialization.hpp>\nint main(){ cpp_utils::serde::deserialize<int>((const char*)nullptr); }' ` would fail to compile (don't add this as a permanent test — bare-pointer-with-no-size is exactly the unsafe form we removed, there's nothing to regression-test going forward since it simply doesn't exist as a valid call anymore).

- [ ] **Step 6: Update root `meson.build`**

Add `'include/cpp_utils/serde/context.hpp'` to the `headers` list in the root `meson.build`, next to the other `serde/` entries.

- [ ] **Step 7: Commit**

```bash
git add include/cpp_utils/serde/context.hpp include/cpp_utils/serde/deserialization.hpp tests/serde/test_deserialization.cpp meson.build
git commit -m "serde: std::span buffer view + threaded context parameter for deserialize"
```

---

### Task 4: Serialization skeleton — `byte_sink`, `encode`, `serialize` for existing field types

**Files:**
- Modify: `include/cpp_utils/endianness/endianness.hpp` (add `encode`/`encode_v`, symmetric to `decode`/`decode_v`)
- Create: `include/cpp_utils/serde/serialization.hpp`
- Modify: `include/cpp_utils/serde/deserialization.hpp` (add the missing `load_field` overload for `padding_bytes_t` — a pre-existing gap, not something this plan introduced)
- Create: `tests/serde/test_serialization.cpp`
- Modify: root `meson.build`, `tests/meson.build`

**Interfaces:**
- Produces: `cpp_utils::serde::byte_sink` (concept); `serialize(value, sink, offset, context)`; `save_field(parent, sink, offset, context, field)` overload set covering fundamental, enum, composite (recursion), `static_array_field`, `dynamic_array_field` (both plain and until-eof — see note below), and `padding_bytes_t`.
- Consumes: `load_field`'s 5-parameter shape and `context.hpp` from Task 3.

Key simplification worth stating up front: **save never needs to ask "how many elements/bytes" — the value already holds the real container, so `save_field` just writes `field.size()` elements/bytes of whatever is actually there.** This is why `dynamic_array_field`'s `save_field` overload is a single overload covering both `dynamic_array<ID,T>` and `dynamic_array_until_eof<T>` (both satisfy the same `dynamic_array_field` concept and both just get written element-by-element) — unlike loading, which has three separate overloads because it has to *predict* a count before the container exists. `context` is still threaded through `serialize` for API symmetry with `deserialize` (per the approved design spec), but none of the field types in this task's scope actually read it.

- [ ] **Step 1: Write the failing round-trip test**

Create `tests/serde/test_serialization.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

TEST_CASE("Serialize", "[simple structures]")
{
    GIVEN("A simple struct with two chars")
    {
        struct two_chars
        {
            char a;
            char b;
        };
        two_chars value { 'c', 'd' };
        WHEN("Serializing it")
        {
            std::vector<uint8_t> sink;
            auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
            THEN("It should produce the right bytes and round-trip through deserialize")
            {
                REQUIRE(end_offset == 2);
                REQUIRE(sink.size() == 2);
                REQUIRE(sink[0] == 'c');
                REQUIRE(sink[1] == 'd');
                auto roundtrip = cpp_utils::serde::deserialize<two_chars>(sink);
                REQUIRE(roundtrip.a == 'c');
                REQUIRE(roundtrip.b == 'd');
            }
        }
    }
}

TEST_CASE("Serialize", "[nested structs]")
{
    struct struct2
    {
        struct struct1
        {
            char a;
            char b;
        } s;
        char c;
    };
    struct2 value { { 'a', 'b' }, 'c' };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink.size() == 3);
    auto roundtrip = cpp_utils::serde::deserialize<struct2>(sink);
    REQUIRE(roundtrip.s.a == 'a');
    REQUIRE(roundtrip.s.b == 'b');
    REQUIRE(roundtrip.c == 'c');
}

TEST_CASE("Serialize", "[big endian fundamental]")
{
    struct big_endian
    {
        using endianness = cpp_utils::endianness::big_endian_t;
        uint32_t a;
        uint16_t b;
    };
    big_endian value { 1, 2 };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 0, 0, 0, 1, 0, 2 });
}

TEST_CASE("Serialize", "[static array]")
{
    struct array_members
    {
        cpp_utils::serde::static_array<uint16_t, 2> a;
        uint16_t b;
    };
    array_members value {};
    value.a[0] = 1;
    value.a[1] = 2;
    value.b = 3;
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 1, 0, 2, 0, 3, 0 });
}

TEST_CASE("Serialize", "[padding]")
{
    struct with_padding
    {
        char a;
        cpp_utils::serde::padding_bytes_t<3, 0xFF> pad;
        char b;
    };
    with_padding value { 'x', {}, 'y' };
    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 'x', 0xFF, 0xFF, 0xFF, 'y' });
}
```

- [ ] **Step 2: Register the test and run it to verify it fails**

Add to `tests/meson.build`'s `tests` list:

```meson
    {
        'name':'TestSerialization',
        'sources': ['serde/test_serialization.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

Run: `meson setup build --reconfigure && ninja -C build`
Expected: **compile failure** — `cpp_utils::serde::serialize` doesn't exist yet.

- [ ] **Step 3: Add `encode`/`encode_v` to `endianness.hpp`**

Append to `include/cpp_utils/endianness/endianness.hpp`, right after the existing `decode_v` overloads (before the closing `}` of `namespace cpp_utils::endianness`):

```cpp
template <typename dst_endianess_t, typename T>
HEDLEY_NON_NULL(2)
inline void encode(T value, char* output)
{
    if constexpr (sizeof(T) > 1)
    {
        if constexpr (!std::is_same_v<host_endianness_t, dst_endianess_t>)
        {
            value = details::byte_swap<T>(value);
        }
        std::memcpy(output, &value, sizeof(T));
    }
    else
    {
        *output = static_cast<char>(value);
    }
}

template <typename dst_endianess_t, typename value_t>
HEDLEY_NON_NULL(1)
inline void encode_v(const value_t* data, std::size_t count, char* output)
{
    if constexpr (sizeof(value_t) > 1 and not std::is_same_v<host_endianness_t, dst_endianess_t>)
    {
        for (auto i = 0UL; i < count; i++)
        {
            const auto swapped = details::byte_swap(data[i]);
            std::memcpy(output + i * sizeof(value_t), &swapped, sizeof(value_t));
        }
    }
    else
    {
        if (count > 0)
            std::memcpy(output, data, count * sizeof(value_t));
    }
}
```

- [ ] **Step 4: Add the missing `padding_bytes_t` load support**

`padding_bytes_t` has never had a `load_field` overload in `deserialization.hpp` — it's a pre-existing gap this task closes since Step 1's round-trip test exercises it. Add to `include/cpp_utils/serde/deserialization.hpp`, next to the other `load_field` overloads (e.g. right after the `static_array_field` one):

```cpp
template <std::size_t size, uint8_t value>
constexpr inline std::size_t load_field(const auto&, auto&, std::size_t offset, const auto&,
    padding_bytes_t<size, value>&)
{
    return offset + size;
}
```

(Matches `padding_bytes_t`'s two template parameters directly — no helper concept needed.)

- [ ] **Step 5: Create `serialization.hpp`**

Create `include/cpp_utils/serde/serialization.hpp` (copy the MIT license header block from `deserialization.hpp:1-26` verbatim, then):

```cpp
#pragma once
#include "../endianness/endianness.hpp"
#include "../reflexion/reflection.hpp"
#include "../types/concepts.hpp"
#include "context.hpp"
#include "special_fields.hpp"
#include <cstddef>
#include <cstring>

namespace cpp_utils::serde
{

template <typename T>
concept byte_sink = requires(T t, std::size_t n) {
    { t.resize(n) };
    { t.data() };
    { t.size() } -> std::convertible_to<std::size_t>;
};

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, serialize, const);

namespace details
{
    void ensure_size(byte_sink auto& sink, std::size_t needed)
    {
        if (sink.size() < needed)
            sink.resize(needed);
    }

    void _save_value_to_memory(byte_sink auto& sink, std::size_t offset,
        types::concepts::fundamental_type auto value, const auto& parent_composite)
    {
        using T = std::decay_t<decltype(value)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        ensure_size(sink, offset + sizeof(T));
        endianness::encode<endianness_t<parent_composite_t>>(
            value, reinterpret_cast<char*>(sink.data()) + offset);
    }

    void _save_values_to_memory(byte_sink auto& sink, std::size_t offset,
        const types::concepts::fundamental_type auto* values, std::size_t count,
        const auto& parent_composite)
    {
        using T = std::decay_t<decltype(*values)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        ensure_size(sink, offset + sizeof(T) * count);
        endianness::encode_v<endianness_t<parent_composite_t>>(
            values, count, reinterpret_cast<char*>(sink.data()) + offset);
    }
}

constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, types::concepts::fundamental_type auto field)
{
    (void)context;
    details::_save_value_to_memory(sink, offset, field, parent_composite);
    return offset + sizeof(field);
}

constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, types::concepts::enum_type auto field)
{
    using underlying_t = std::underlying_type_t<std::decay_t<decltype(field)>>;
    return save_field(
        parent_composite, sink, offset, context, static_cast<underlying_t>(field));
}

constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, const dynamic_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = array_field.size();
    if (count == 0)
        return offset;
    if constexpr (std::is_compound_v<field_t>)
    {
        for (std::size_t i = 0; i < count; ++i)
            offset = serialize(array_field[i], sink, offset, context);
        return offset;
    }
    else
    {
        details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
        return offset + sizeof(field_t) * count;
    }
}

constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, const static_array_field auto& array_field)
{
    (void)context;
    using array_field_t = std::decay_t<decltype(array_field)>;
    constexpr auto count = array_field_t::array_size;
    using field_t = typename array_field_t::value_type;
    details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
    return offset + sizeof(field_t) * count;
}

template <std::size_t size, uint8_t value>
constexpr inline std::size_t save_field(const auto&, byte_sink auto& sink, std::size_t offset,
    const auto&, const padding_bytes_t<size, value>&)
{
    details::ensure_size(sink, offset + size);
    std::memset(reinterpret_cast<char*>(sink.data()) + offset, value, size);
    return offset + size;
}

template <typename composite_t, typename sink_t, typename context_t, typename T>
constexpr inline std::size_t save_fields(const composite_t& r, sink_t& sink,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return serialize(field, sink, offset, context);
    else
        return save_field(r, sink, offset, context, std::forward<T>(field));
}

template <typename composite_t, typename sink_t, typename context_t, typename T, typename... Ts>
constexpr inline std::size_t save_fields(const composite_t& r, sink_t& sink,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field, Ts&&... fields)
{
    offset = save_fields(r, sink, offset, context, std::forward<T>(field));
    return save_fields(r, sink, offset, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, serialize, save_fields, const);

template <typename composite_t, byte_sink sink_t>
constexpr std::size_t serialize(const composite_t& value, sink_t& sink,
    std::size_t offset = 0, const auto& context = no_context {})
{
    return serialize(value, sink, offset, context);
}

}
```

Note the final `serialize` overload just forwards to the `SPLIT_FIELDS`-generated one with defaulted `offset`/`context` — mirrors how `deserialize<composite_t>` provides defaults on top of the macro-generated function in `deserialization.hpp`. (It's a legal overload, not infinite recursion: the macro-generated `serialize(const composite_t&, sink_t&, std::size_t, const context_t&)` has no default arguments and 4 required parameters, while this one has defaults and template-deduces differently — calls with fewer than 4 explicit arguments resolve to this one, which then calls the 4-arg one explicitly.)

- [ ] **Step 6: Add `serialization.hpp` to `serde.hpp` and to the root `meson.build` headers list**

`include/cpp_utils/serde/serde.hpp` becomes:

```cpp
#pragma once
#include "special_fields.hpp"
#include "deserialization.hpp"
#include "serialization.hpp"
```

Add `'include/cpp_utils/serde/serialization.hpp'` to the `headers` list in the root `meson.build`.

- [ ] **Step 7: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the new `Test-TestSerialization`.

- [ ] **Step 8: Commit**

```bash
git add include/cpp_utils/endianness/endianness.hpp include/cpp_utils/serde/deserialization.hpp include/cpp_utils/serde/serialization.hpp include/cpp_utils/serde/serde.hpp tests/serde/test_serialization.cpp tests/meson.build meson.build
git commit -m "serde: add serialize/save_field, symmetric to deserialize/load_field"
```

---

### Task 5: `bounded_string<N>` field type

**Files:**
- Modify: `include/cpp_utils/reflexion/reflection.hpp` (add the `wire_size` customization point to `field_size()`)
- Modify: `include/cpp_utils/serde/special_fields.hpp` (add `bounded_string<N>`)
- Modify: `include/cpp_utils/serde/deserialization.hpp` (add `load_field` overload)
- Modify: `include/cpp_utils/serde/serialization.hpp` (add `save_field` overload)
- Create: `tests/serde/test_bounded_string.cpp`
- Modify: `tests/meson.build`

**Interfaces:**
- Produces: `cpp_utils::serde::bounded_string<N>` with public member `std::string value` and `static constexpr std::size_t max_len = N`.

Background for the `wire_size` customization point: `reflexion::field_size<field_t>()` currently falls back to `sizeof(field_t)` for any non-composite, non-dynamic field. That's correct for `static_array<T,N>` (its only member is a `T[N]`, so `sizeof` naturally equals the wire size), but wrong for `bounded_string<N>`: it holds a `std::string`, whose `sizeof` (some small-object-optimized struct, unrelated to N) has nothing to do with its N-byte wire footprint. This step adds an opt-in `static constexpr std::size_t wire_size` that, when present, `field_size()` prefers over `sizeof`.

- [ ] **Step 1: Write the failing test**

Create `tests/serde/test_bounded_string.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <string>
#include <vector>

TEST_CASE("bounded_string load", "[serde]")
{
    struct with_name
    {
        cpp_utils::serde::bounded_string<8> name;
        uint16_t value;
    };
    std::vector<uint8_t> buffer { 'a', 'b', 'c', 0, 0, 0, 0, 0, 5, 0 };
    auto s = cpp_utils::serde::deserialize<with_name>(buffer);
    REQUIRE(s.name.value == "abc");
    REQUIRE(s.value == 5);
}

TEST_CASE("bounded_string load stops at embedded null even mid-buffer", "[serde]")
{
    struct just_name
    {
        cpp_utils::serde::bounded_string<4> name;
    };
    std::vector<uint8_t> buffer { 'h', 'i', 0, 'z' };
    auto s = cpp_utils::serde::deserialize<just_name>(buffer);
    REQUIRE(s.name.value == "hi");
}

TEST_CASE("bounded_string round-trip via serialize", "[serde]")
{
    struct with_name
    {
        cpp_utils::serde::bounded_string<8> name;
        uint16_t value;
    };
    with_name original;
    original.name.value = "abc";
    original.value = 5;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(original, sink, std::size_t { 0 });
    REQUIRE(end_offset == 10);
    REQUIRE(sink == std::vector<uint8_t> { 'a', 'b', 'c', 0, 0, 0, 0, 0, 5, 0 });

    auto roundtrip = cpp_utils::serde::deserialize<with_name>(sink);
    REQUIRE(roundtrip.name.value == "abc");
    REQUIRE(roundtrip.value == 5);
}

static_assert(cpp_utils::reflexion::field_size<cpp_utils::serde::bounded_string<8>>() == 8);
static_assert(cpp_utils::reflexion::const_size_field<cpp_utils::serde::bounded_string<8>>);
```

Register in `tests/meson.build`:

```meson
    {
        'name':'TestBoundedString',
        'sources': ['serde/test_bounded_string.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

- [ ] **Step 2: Run it to verify it fails**

Run: `meson setup build --reconfigure && ninja -C build`
Expected: **compile failure** — `cpp_utils::serde::bounded_string` doesn't exist yet.

- [ ] **Step 3: Fix `_has_const_size` to respect `do_not_split`, then add the `wire_size` customization point**

There's a pre-existing latent bug this type exposes: `reflexion::const_size_field<T>` (and the
`_has_const_size` helper behind it) recurses into `composite_have_const_size<T>` — which
structured-binds `T`'s own members — for *any* compound type, without first checking whether
`T` has opted out of splitting via `do_not_split`. `dynamic_array<ID,T>` never hit this because
it also carries `dyn_size_field_tag`, which makes `composite_have_const_size` return `false`
*before* it ever tries to destructure `dynamic_array`'s private `std::vector` member. `static_array<T,N>`
would hit it too if `const_size_field<static_array<T,N>>` were ever evaluated directly (its
`_data` array member is private, so structured-binding it is ill-formed) — it just happens that
nothing in the current codebase evaluates that concept on a `do_not_split` type directly. Since
`bounded_string<N>` is constant-size (no `dyn_size_field_tag`) and wraps a `std::string` (which
has private members and is not itself a structured-binding-compatible aggregate), evaluating
`const_size_field<bounded_string<N>>` — which Task 5's own test does, and which Task 7's
`unused<T>` does internally for every `T` it wraps — would try to structured-bind a
`std::string` and fail to compile.

Fix `_has_const_size` in `include/cpp_utils/reflexion/reflection.hpp` to check `can_split_v`
first, the same way `fields_have_const_size` already does correctly for fields nested inside a
struct. Change:

```cpp
template <typename field_t>
consteval bool _has_const_size()
{
    if constexpr (std::is_compound_v<field_t>)
    {
        return reflexion::composite_have_const_size<field_t>();
    }
    else
    {
        return !reflexion::is_dyn_size_field_v<std::decay_t<field_t>>;
    }
}
```

to:

```cpp
template <typename field_t>
consteval bool _has_const_size()
{
    if constexpr (std::is_compound_v<field_t> && can_split_v<field_t>)
    {
        return reflexion::composite_have_const_size<field_t>();
    }
    else
    {
        return !reflexion::is_dyn_size_field_v<std::decay_t<field_t>>;
    }
}
```

This only changes behavior for `do_not_split`-tagged compound types (now correctly resolved via
their `dyn_size_field_tag` presence/absence, without ever attempting to destructure their
internals) — genuinely splittable composites (plain structs with no `do_not_split`) go through
the exact same `composite_have_const_size` path as before, so no existing test is affected.

Next, add the `wire_size` customization point. Modify `field_size()` (the function currently reading):

```cpp
template <typename field_t>
[[nodiscard]] consteval std::size_t field_size()
{
    using Field_t = std::decay_t<field_t>;
    static_assert(!is_dyn_size_field_v<Field_t>, "Dynamic size fields are not supported here");
    if constexpr (std::is_bounded_array_v<Field_t>)
        return sizeof(Field_t) * std::extent_v<Field_t>;
    if constexpr (can_split_v<Field_t>)
        return composite_size<Field_t>();
    return sizeof(Field_t);
}
```

to:

```cpp
template <typename field_t>
[[nodiscard]] consteval std::size_t field_size()
{
    using Field_t = std::decay_t<field_t>;
    static_assert(!is_dyn_size_field_v<Field_t>, "Dynamic size fields are not supported here");
    if constexpr (std::is_bounded_array_v<Field_t>)
        return sizeof(Field_t) * std::extent_v<Field_t>;
    if constexpr (requires { Field_t::wire_size; })
        return Field_t::wire_size;
    if constexpr (can_split_v<Field_t>)
        return composite_size<Field_t>();
    return sizeof(Field_t);
}
```

While here, close the same gap for `padding_bytes_t`: it has no data members, so `sizeof(padding_bytes_t<size,value>)` (the fallback this function used before today) is 1, not `size` — wrong whenever `reflexion::field_size`/`composite_size` is computed for a struct containing a padding field (not exercised by any test yet, but worth closing now that the mechanism exists). In `include/cpp_utils/serde/special_fields.hpp`, add a `wire_size` member to the existing `padding_bytes_t`:

```cpp
template <std::size_t size, uint8_t value>
struct padding_bytes_t
{
    using do_not_split = reflexion::do_not_split_t;
    static constexpr std::size_t padding_size = size;
    static constexpr std::size_t wire_size = size;
    static constexpr uint8_t padding_value = value;
};
```

(Only the added `wire_size` line is new — `padding_size`/`padding_value` stay as they are.)

- [ ] **Step 4: Add `bounded_string<N>` to `special_fields.hpp`**

Add `#include <string>` to the includes at the top of `include/cpp_utils/serde/special_fields.hpp`. Then add, next to `static_array`:

```cpp
template <std::size_t N>
struct bounded_string
{
    using do_not_split = reflexion::do_not_split_t;
    static constexpr std::size_t wire_size = N;
    static constexpr std::size_t max_len = N;
    std::string value;
};

template <typename T>
concept bounded_string_field = std::is_same_v<T, bounded_string<T::max_len>>;
```

- [ ] **Step 5: Add the `load_field` overload**

Add to `include/cpp_utils/serde/deserialization.hpp`, next to the `static_array_field` overload:

```cpp
constexpr inline std::size_t load_field(const auto&, auto& parsing_context, std::size_t offset,
    const auto&, bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    const auto bytes = details::to_byte_view(parsing_context);
    std::size_t len = 0;
    while (len < field_t::max_len
        && std::to_integer<unsigned char>(bytes[offset + len]) != 0)
        ++len;
    field.value = std::string {
        reinterpret_cast<const char*>(bytes.data()) + offset, len
    };
    return offset + field_t::max_len;
}
```

- [ ] **Step 6: Add the `save_field` overload**

Add to `include/cpp_utils/serde/serialization.hpp`, next to the `static_array_field` overload:

```cpp
constexpr inline std::size_t save_field(const auto&, byte_sink auto& sink, std::size_t offset,
    const auto&, const bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    details::ensure_size(sink, offset + field_t::max_len);
    auto* out = reinterpret_cast<char*>(sink.data()) + offset;
    const auto copy_len = std::min(field.value.size(), field_t::max_len);
    std::memcpy(out, field.value.data(), copy_len);
    if (copy_len < field_t::max_len)
        std::memset(out + copy_len, 0, field_t::max_len - copy_len);
    return offset + field_t::max_len;
}
```

(`serialization.hpp` already includes `<cstring>` from Task 4; if `std::min` isn't already visible, add `#include <algorithm>` to its includes.)

- [ ] **Step 7: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the new `Test-TestBoundedString`.

- [ ] **Step 8: Commit**

```bash
git add include/cpp_utils/reflexion/reflection.hpp include/cpp_utils/serde/special_fields.hpp include/cpp_utils/serde/deserialization.hpp include/cpp_utils/serde/serialization.hpp tests/serde/test_bounded_string.cpp tests/meson.build
git commit -m "serde: add bounded_string<N> field type and reflexion wire_size customization point"
```

---

### Task 6: `dynamic_array_bytes<ID, T>` field type

**Files:**
- Modify: `include/cpp_utils/serde/special_fields.hpp`
- Modify: `include/cpp_utils/serde/deserialization.hpp`
- Modify: `include/cpp_utils/serde/serialization.hpp`
- Create: `tests/serde/test_dynamic_array_bytes.cpp`
- Modify: `tests/meson.build`

**Interfaces:**
- Produces: `cpp_utils::serde::dynamic_array_bytes<ID, T>` — same public shape as `dynamic_array<ID, T>` (`operator[]`, `data()`, `size()`, `resize()`, iterators), but its parent record's `field_size(field, ...)` callback is interpreted as a **byte** length rather than an element count.

This is the type that exercises `context` threading for real: it demonstrates the "a field's size depends on a sibling field elsewhere, not on the immediate parent record" scenario (CDFpp's motivating case — a `zVDR`-shaped record whose array field length depends on a `GDR`-shaped record's `rNumDims`, reached through the context object).

- [ ] **Step 1: Write the failing test**

Create `tests/serde/test_dynamic_array_bytes.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("dynamic_array_bytes sized from the parent record itself", "[serde]")
{
    struct with_table
    {
        uint16_t count_bytes;
        cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    std::vector<uint8_t> buffer { 4, 0, 1, 0, 2, 0 };
    auto s = cpp_utils::serde::deserialize<with_table>(buffer);
    REQUIRE(s.values.size() == 2);
    REQUIRE(s.values[0] == 1);
    REQUIRE(s.values[1] == 2);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(end_offset == 6);
    REQUIRE(sink == buffer);
}

TEST_CASE("dynamic_array_bytes sized from an external context (CDFpp's rNumDims case)", "[serde]")
{
    struct dims_context
    {
        int32_t num_dims;
    };
    struct rvdr_like
    {
        cpp_utils::serde::dynamic_array_bytes<0, int32_t> dim_varys;

        std::size_t field_size(const cpp_utils::serde::dynamic_array_bytes<0, int32_t>&,
            const dims_context& ctx) const
        {
            return static_cast<std::size_t>(ctx.num_dims) * sizeof(int32_t);
        }
    };
    std::vector<uint8_t> buffer { 1, 0, 0, 0, 0, 0, 0, 0 };
    dims_context ctx { 2 };
    auto s = cpp_utils::serde::deserialize<rvdr_like>(buffer, 0, ctx);
    REQUIRE(s.dim_varys.size() == 2);
    REQUIRE(s.dim_varys[0] == 1);
    REQUIRE(s.dim_varys[1] == 0);
}
```

Register in `tests/meson.build`:

```meson
    {
        'name':'TestDynamicArrayBytes',
        'sources': ['serde/test_dynamic_array_bytes.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

- [ ] **Step 2: Run it to verify it fails**

Run: `meson setup build --reconfigure && ninja -C build`
Expected: **compile failure** — `cpp_utils::serde::dynamic_array_bytes` doesn't exist yet.

- [ ] **Step 3: Add `dynamic_array_bytes<ID, T>` to `special_fields.hpp`**

Add, next to `dynamic_array`:

```cpp
template <std::size_t ID, typename field_t>
struct dynamic_array_bytes : dynamic_array<ID, field_t>
{
};

template <typename T>
concept dynamic_array_bytes_field
    = std::is_same_v<T, dynamic_array_bytes<T::id, typename T::value_type>>;
```

(Reuses `dynamic_array`'s storage and interface entirely via inheritance — it is a distinct type only so `load_field`/`save_field` overload resolution can tell it apart from element-counted `dynamic_array`. It already inherits `do_not_split` and `dyn_size_field_tag` from the base.)

- [ ] **Step 4: Add the `load_field` overload**

Add to `include/cpp_utils/serde/deserialization.hpp`, next to the `dyn_array_field` overload:

```cpp
constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dynamic_array_bytes_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto bytes = details::resolve_field_size(parent_composite, array_field, context);
    const auto count = bytes / sizeof(field_t);
    if (count > 0)
    {
        array_field.resize(count);
        details::_load_values_from_memory(details::to_byte_view(parsing_context), offset,
            array_field.data(), count, parent_composite);
    }
    return offset + bytes;
}
```

- [ ] **Step 5: Add the `save_field` overload**

Add to `include/cpp_utils/serde/serialization.hpp`, next to the `dynamic_array_field` overload:

```cpp
constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, const dynamic_array_bytes_field auto& array_field)
{
    (void)context;
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = array_field.size();
    if (count > 0)
        details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
    return offset + sizeof(field_t) * count;
}
```

- [ ] **Step 6: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the new `Test-TestDynamicArrayBytes`.

- [ ] **Step 7: Commit**

```bash
git add include/cpp_utils/serde/special_fields.hpp include/cpp_utils/serde/deserialization.hpp include/cpp_utils/serde/serialization.hpp tests/serde/test_dynamic_array_bytes.cpp tests/meson.build
git commit -m "serde: add dynamic_array_bytes<ID,T>, a byte-length-sized dynamic field"
```

---

### Task 7: `unused<T>` field type

**Files:**
- Modify: `include/cpp_utils/serde/special_fields.hpp`
- Modify: `include/cpp_utils/serde/deserialization.hpp`
- Modify: `include/cpp_utils/serde/serialization.hpp`
- Create: `tests/serde/test_unused_field.cpp`
- Modify: `tests/meson.build`

**Interfaces:**
- Produces: `cpp_utils::serde::unused<T>` — consumes/produces the same bytes as `T`, discards the decoded value on load, writes zero-filled bytes on save.

Scope boundary, stated explicitly (not a placeholder): **`unused<T>::save_field` only supports constant-size `T`** (enforced by `static_assert(reflexion::const_size_field<T>, ...)`). Loading works for *any* `T`, fixed or dynamic, because loading only needs to consume the right number of bytes — which it gets by delegating to `T`'s own `load_field` and throwing the result away, exactly the same mechanism whether `T`'s size comes from `sizeof` or from context resolution. Saving a discarded *dynamic*-size field would need to know how many bytes *should have been* reserved without a real value to measure — CDFpp's own only occurrence of this pattern (`unused_field<table_field<char,2>>`) happens to use a hardcoded constant size (132), not genuine runtime-dependent sizing, so this boundary doesn't block real usage; extending it is left for whenever the CDFpp cutover project actually needs it.

- [ ] **Step 1: Write the failing test**

Create `tests/serde/test_unused_field.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("unused<T> load discards the value but consumes the right bytes", "[serde]")
{
    struct with_reserved
    {
        char a;
        cpp_utils::serde::unused<int32_t> reserved;
        char b;
    };
    std::vector<uint8_t> buffer { 'x', 0xDE, 0xAD, 0xBE, 0xEF, 'y' };
    auto s = cpp_utils::serde::deserialize<with_reserved>(buffer);
    REQUIRE(s.a == 'x');
    REQUIRE(s.b == 'y');
}

TEST_CASE("unused<T> save writes zero-filled bytes of T's size", "[serde]")
{
    struct with_reserved
    {
        char a;
        cpp_utils::serde::unused<int32_t> reserved;
        char b;
    };
    with_reserved value { 'x', {}, 'y' };
    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(end_offset == 6);
    REQUIRE(sink == std::vector<uint8_t> { 'x', 0, 0, 0, 0, 'y' });
}

TEST_CASE("unused<T> wrapping a bounded_string round-trips", "[serde]")
{
    struct with_reserved_name
    {
        cpp_utils::serde::unused<cpp_utils::serde::bounded_string<4>> reserved;
        char tail;
    };
    std::vector<uint8_t> buffer { 'h', 'i', 0, 0, 'z' };
    auto s = cpp_utils::serde::deserialize<with_reserved_name>(buffer);
    REQUIRE(s.tail == 'z');

    std::vector<uint8_t> sink;
    cpp_utils::serde::serialize(s, sink, std::size_t { 0 });
    REQUIRE(sink == std::vector<uint8_t> { 0, 0, 0, 0, 'z' });
}

TEST_CASE("unused<T> load discards a genuinely dynamic-size inner field too", "[serde]")
{
    struct with_reserved_table
    {
        uint16_t count_bytes;
        cpp_utils::serde::unused<cpp_utils::serde::dynamic_array_bytes<0, uint16_t>> reserved;
        char tail;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    std::vector<uint8_t> buffer { 4, 0, 1, 0, 2, 0, 'z' };
    auto s = cpp_utils::serde::deserialize<with_reserved_table>(buffer);
    REQUIRE(s.tail == 'z');
    // save_field intentionally does not support unused<T> for dynamic-size T (see
    // static_assert in save_field) — only the load direction is exercised here.
}
```

Register in `tests/meson.build`:

```meson
    {
        'name':'TestUnusedField',
        'sources': ['serde/test_unused_field.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

- [ ] **Step 2: Run it to verify it fails**

Run: `meson setup build --reconfigure && ninja -C build`
Expected: **compile failure** — `cpp_utils::serde::unused` doesn't exist yet.

- [ ] **Step 3: Add `unused<T>` to `special_fields.hpp`**

Add, near the end of the file (after `dynamic_array_of_constant_size_field`, before the `std` specializations block):

```cpp
namespace details
{
    struct empty_base
    {
    };
    struct dyn_size_base
    {
        using dyn_size_field_tag = reflexion::dyn_size_field_tag_t;
    };
}

template <typename T>
struct unused : std::conditional_t<!const_size_field<T>, details::dyn_size_base, details::empty_base>
{
    using do_not_split = reflexion::do_not_split_t;
    using value_type = T;
};

template <typename T>
concept unused_field = std::is_same_v<T, unused<typename T::value_type>>;
```

- [ ] **Step 4: Add the `load_field` overload**

Add to `include/cpp_utils/serde/deserialization.hpp`, next to the `bounded_string_field` overload:

```cpp
constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, unused_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    typename field_t::value_type discarded {};
    return load_field(parent_composite, parsing_context, offset, context, discarded);
}
```

- [ ] **Step 5: Add the `save_field` overload**

Add to `include/cpp_utils/serde/serialization.hpp`, next to the `bounded_string_field` overload:

```cpp
constexpr inline std::size_t save_field(const auto& parent_composite, byte_sink auto& sink,
    std::size_t offset, const auto& context, const unused_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    using value_type = typename field_t::value_type;
    static_assert(reflexion::const_size_field<value_type>,
        "cpp_utils::serde::unused<T>: T must be a constant-size field; dynamic-size T is not "
        "yet supported by save_field");
    value_type zero {};
    return save_field(parent_composite, sink, offset, context, zero);
}
```

- [ ] **Step 6: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the new `Test-TestUnusedField`.

- [ ] **Step 7: Commit**

```bash
git add include/cpp_utils/serde/special_fields.hpp include/cpp_utils/serde/deserialization.hpp include/cpp_utils/serde/serialization.hpp tests/serde/test_unused_field.cpp tests/meson.build
git commit -m "serde: add unused<T> discard-wrapper field type"
```

---

### Task 8: `runtime_size<T>`

**Files:**
- Create: `include/cpp_utils/serde/runtime_size.hpp`
- Modify: `include/cpp_utils/serde/serde.hpp`
- Create: `tests/serde/test_runtime_size.cpp`
- Modify: root `meson.build`, `tests/meson.build`

**Interfaces:**
- Produces: `cpp_utils::serde::runtime_size<T>(const T& value, const context_t& context = no_context{})` — returns the exact number of bytes `serialize` would produce for `value`, without writing anything.

Like `save_field`, `runtime_size` never needs to resolve a field's size from the parent/context — the value already exists, so measuring is just "sum up how many bytes each field's actual content occupies." For const-size fields that's `reflexion::field_size<field_t>()` (a `consteval` compile-time constant); for dynamic fields it's `field.size() * sizeof(element)` read directly off the real container.

- [ ] **Step 1: Write the failing test**

Create `tests/serde/test_runtime_size.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <serde/serde.hpp>
#include <vector>

TEST_CASE("runtime_size matches serialize output length for const-size composites", "[serde]")
{
    struct struct2
    {
        struct struct1
        {
            char a;
            char b;
        } s;
        char c;
    };
    struct2 value { { 'a', 'b' }, 'c' };
    REQUIRE(cpp_utils::serde::runtime_size(value) == 3);

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(cpp_utils::serde::runtime_size(value) == end_offset);
}

TEST_CASE("runtime_size matches serialize output length for dynamic fields", "[serde]")
{
    struct with_table
    {
        uint16_t count_bytes;
        cpp_utils::serde::dynamic_array_bytes<0, uint16_t> values;

        std::size_t field_size(
            const cpp_utils::serde::dynamic_array_bytes<0, uint16_t>&) const
        {
            return count_bytes;
        }
    };
    with_table value {};
    value.count_bytes = 4;
    value.values.resize(2);
    value.values[0] = 1;
    value.values[1] = 2;

    std::vector<uint8_t> sink;
    auto end_offset = cpp_utils::serde::serialize(value, sink, std::size_t { 0 });
    REQUIRE(cpp_utils::serde::runtime_size(value) == end_offset);
    REQUIRE(cpp_utils::serde::runtime_size(value) == 6);
}
```

Register in `tests/meson.build`:

```meson
    {
        'name':'TestRuntimeSize',
        'sources': ['serde/test_runtime_size.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
```

- [ ] **Step 2: Run it to verify it fails**

Run: `meson setup build --reconfigure && ninja -C build`
Expected: **compile failure** — `cpp_utils::serde::runtime_size` doesn't exist yet.

- [ ] **Step 3: Create `runtime_size.hpp`**

Create `include/cpp_utils/serde/runtime_size.hpp` (copy the MIT license header block from `serialization.hpp` verbatim, then):

```cpp
#pragma once
#include "../reflexion/reflection.hpp"
#include "context.hpp"
#include "special_fields.hpp"

namespace cpp_utils::serde
{

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, runtime_size, const);

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const types::concepts::fundamental_type auto& field)
{
    return sizeof(field);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const types::concepts::enum_type auto& field)
{
    return sizeof(field);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const dynamic_array_field auto& array_field)
{
    using field_t = typename std::decay_t<decltype(array_field)>::value_type;
    return array_field.size() * reflexion::field_size<field_t>();
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const dynamic_array_bytes_field auto& array_field)
{
    using field_t = typename std::decay_t<decltype(array_field)>::value_type;
    return array_field.size() * sizeof(field_t);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const static_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    return array_field_t::array_size * sizeof(field_t);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    return field_t::max_len;
}

template <std::size_t size, uint8_t value>
constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const padding_bytes_t<size, value>&)
{
    return size;
}

constexpr inline std::size_t field_runtime_size(
    const auto& parent, const auto& context, const unused_field auto& field)
{
    using value_type = typename std::decay_t<decltype(field)>::value_type;
    value_type zero {};
    return field_runtime_size(parent, context, zero);
}

template <typename composite_t, typename context_t, typename T>
constexpr inline std::size_t fields_runtime_size(
    const composite_t& r, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return runtime_size(field, context);
    else
        return field_runtime_size(r, context, std::forward<T>(field));
}

template <typename composite_t, typename context_t, typename T, typename... Ts>
constexpr inline std::size_t fields_runtime_size(
    const composite_t& r, const context_t& context, T&& field, Ts&&... fields)
{
    return fields_runtime_size(r, context, std::forward<T>(field))
        + fields_runtime_size(r, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, runtime_size, fields_runtime_size, const);

}
```

- [ ] **Step 4: Wire it into `serde.hpp`**

`include/cpp_utils/serde/serde.hpp` becomes:

```cpp
#pragma once
#include "special_fields.hpp"
#include "deserialization.hpp"
#include "serialization.hpp"
#include "runtime_size.hpp"
```

Add `'include/cpp_utils/serde/runtime_size.hpp'` to the `headers` list in the root `meson.build`.

- [ ] **Step 5: Run the test suite**

Run: `ninja -C build && meson test -C build`
Expected: all tests pass, including the new `Test-TestRuntimeSize`.

- [ ] **Step 6: Commit**

```bash
git add include/cpp_utils/serde/runtime_size.hpp include/cpp_utils/serde/serde.hpp tests/serde/test_runtime_size.cpp tests/meson.build meson.build
git commit -m "serde: add runtime_size, computing a value's exact wire length without writing it"
```

---

### Task 9: Documentation refresh + full verification

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:** none (documentation-only task).

- [ ] **Step 1: Update `CLAUDE.md`'s Serialization section**

In `CLAUDE.md`, replace the `### Serialization (serde/)` section with:

```markdown
### Serialization (`serde/`)

`serde::deserialize<T>(input, offset = 0, context = no_context{})` and
`serde::serialize(value, sink, offset = 0, context = no_context{})` are symmetric: both
walk a composite's fields via the reflection machinery (composite fields recurse,
fundamental/enum fields go through `endianness::decode`/`encode`), and both accept an
optional `context` object threaded into every recursive call — used when a dynamic
field's size depends on data outside its immediate parent struct (resolved via
`parent.field_size(field, context)`, falling back to `parent.field_size(field)` when no
context is needed). `serde::runtime_size<T>(value, context)` computes the exact byte
length `serialize` would produce, without writing anything — needed to fill in a
record's own size header before writing it.

Buffers are `std::span`-based throughout (anything span-constructible — containers,
arrays — converts implicitly; a bare pointer with no known size is rejected at compile
time rather than causing an unbounded read). Write targets satisfy the `byte_sink`
concept (`resize`/`data`/`size`-shaped — `std::vector<uint8_t>` and `std::string` both
qualify), not a virtual interface.

Per-struct byte order is opt-in via a nested `using endianness = endianness::big_endian_t;`
on the composite (default little-endian).

`serde/special_fields.hpp` defines the field wrapper types used in on-the-wire structs:
- `static_array<T, N>` — fixed-size array field.
- `dynamic_array<ID, T>` — resizable array field sized via
  `parent_composite.field_size(array_field, [context])`, interpreted as an **element
  count**.
- `dynamic_array_bytes<ID, T>` — same as `dynamic_array`, but the size callback's return
  value is interpreted as a **byte** length instead.
- `dynamic_array_until_eof<T>` (`dynamic_array<SIZE_MAX, T>`) — consumes the rest of the
  buffer.
- `bounded_string<N>` — a `std::string` field read up to a null terminator within N
  bytes, zero-padded to N bytes on write. Declares a `wire_size` member so
  `reflexion::field_size` reports N instead of `sizeof(std::string)`.
- `unused<T>` — wraps any other field type; the decoded value is discarded on load and
  zero-filled on save. `save_field` only supports constant-size `T` (enforced by
  `static_assert`) — a discarded dynamic-size field has no real value to measure from.
- `padding_bytes_t<size, value>` — fixed-size skip/fill field.

When adding a new field wrapper type, give it `do_not_split` (and `dyn_size_field_tag` if
its size isn't known at compile time) and add matching `load_field`/`save_field`
overloads constrained by a concept — don't special-case it inside `load_fields`/
`save_fields`.
```

Also update the `### Reflection (reflexion/reflection.hpp)` section's paragraph about
`MemberCounter`, replacing:

```markdown
The `details::MemberCounter` template (an `anything`-convertible probe object plus recursive
`consteval` search) counts an aggregate's members at compile time without macros or codegen.
```

with:

```markdown
The `details::MemberCounter` template (an `anything`-convertible probe object plus a
`consteval` binary search over `[0, 31]`) counts an aggregate's members at compile time
without macros or codegen. A type needing more than 31 members is a hard compile error
from `count_members` itself, not just from `SPLIT_FIELDS`.
```

- [ ] **Step 2: Run the entire test suite one final time**

Run: `meson setup build --reconfigure && ninja -C build && meson test -C build`
Expected: every test passes (read the actual pass/fail count and exit code from the
output — don't infer success from a partial grep). This should include, at minimum:
`Test-TreePrint`, `Test-TestMemberDetector`, `Test-TestMethodDetector`,
`Test-TestTypeDetector`, `Test-TestSmartPointers`, `Test-TestConcepts`,
`Test-TestContainersTraits`, `Test-TestContainersAlgs`, `Test-TestStringsAlgs`,
`Test-TestEndianness`, `Test-TestLifetime`, `Test-TestDeserialization`,
`Test-TestCountMembers`, `Test-TestSerialization`, `Test-TestBoundedString`,
`Test-TestDynamicArrayBytes`, `Test-TestUnusedField`, `Test-TestRuntimeSize`.

- [ ] **Step 3: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: refresh CLAUDE.md for the unified reflexion/serde engine"
```
