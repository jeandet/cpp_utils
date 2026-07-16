# Unified reflexion + serde engine

Date: 2026-07-15
Status: approved (design), pending implementation plan

## Problem

`cpp_utils` (`include/cpp_utils/reflexion/`, `include/cpp_utils/serde/`) and CDFpp
(`include/cdfpp/cdf-io/reflection.hpp`, `special-fields.hpp`, `records-loading.hpp`,
`records-saving.hpp`) each independently implement the same core trick — C++20
structured-binding-based aggregate reflection used to walk struct fields for binary
(de)serialization — and have drifted:

- **CDFpp silently mis-counts structs with more than 31 members.** Its binary-search
  `count_members` only probes N in `[0, 32)`; aggregate init tolerates fewer initializers
  than members (the rest value-initialize), so the search predicate is true for every
  N ≤ actual member count. For a 40-member struct it returns 31 — silently wrong — instead
  of erroring. `cpp_utils`'s linear `MemberCounter` has no cap and correctly trips
  `static_assert(count <= 31)` in `SPLIT_FIELDS` for oversized structs.
- **CDFpp's `anything` probe lacks the bounded-array SFINAE guard `cpp_utils` has.**
  `cpp_utils::reflexion::details::anything::operator T()` is constrained with
  `requires(!std::is_bounded_array_v<std::decay_t<T>>)`; CDFpp's is unconstrained.
  Per `[class.conv.fct]`, a conversion function's declared type must not be an array —
  instantiating `operator T()` for `T = int[4]` is ill-formed, and whether that's
  SFINAE-friendly is compiler-dependent. Currently dormant (no CDFpp record has a bare
  C-array member) but a latent portability risk given CDFpp explicitly targets MSVC.
- **`cpp_utils`'s `MemberCounter` is O(N), not O(log N), and contains a dead code path.**
  Tracing the `A0`/`c0` split: the branch that would grow `c0` is provably unreachable
  (with `c0` empty, the two `requires`-expressions it distinguishes between are
  syntactically identical), so it always grows `A0` one member at a time — a linear scan
  wearing a two-track disguise. CDFpp's `maximize`/`can_construct_with_N` is a genuine
  binary search.

Both libraries are maintained by the same author for the same class of problem (parsing
binary record formats). The goal is one correct, general engine in `cpp_utils` that both
CDFpp and future consumers can build on, instead of two copies that silently diverge.

## Scope

This spec covers **building the unified engine in `cpp_utils` only**, validated by tests
that replicate CDFpp's real field-parsing requirements. It does **not** cover migrating
CDFpp to depend on it — that is a separate, follow-up project once the engine exists and
is proven. Rationale: the engine and the CDFpp cutover are each substantial, separately
risky pieces of work; decoupling them means CDFpp keeps working unmodified while the
engine is built, and the cutover can be its own scoped, reviewable, incremental effort
later.

The engine unifies the **full load/save pipeline** — `cpp_utils::serde` currently only
has `deserialize`; this adds a symmetric `serialize`, plus a runtime size-computation
path, so CDFpp's record loading *and* saving could eventually both depend on it.

### Non-goals

- CDFpp's actual cutover to depend on this engine (future spec).
- Raising the 31-member `SPLIT_FIELDS` cap.
- Buffer bounds-checking / hardening against corrupt or truncated input. Both engines
  currently assume the caller has already validated buffer size before calling
  deserialize; that assumption is unchanged here.
- Any C++23-only language or library feature (see Constraints).

## Constraints

**C++20 only, strictly.** No `std::byteswap`, `if consteval`, deducing-this,
`std::expected`, or any other C++23-only construct anywhere in the new engine. Both
`cpp_utils` and CDFpp hard-pin `cpp_std=c++20` in their `meson.build` — this isn't a
matter of toolchain support varying across `cibuildwheel`'s manylinux/macOS/Windows
matrix, it's a flat compile-time ceiling from the build configuration itself. Raising it
would be a separate, deliberate decision (affecting both projects, and CDFpp's wheel
build matrix), not something this engine should do unilaterally.

## Design

### 1. Reflexion core (`reflexion/reflection.hpp`)

Replace `details::MemberCounter` with a recursive `consteval` function template doing an
actual binary search over `[Lo, Hi]`, expressed as ordinary `if constexpr`-guarded
recursion rather than CDFpp's template-struct-inheritance `maximize` chain (that pattern
predates `consteval` and is harder to read than it needs to be now):

```cpp
template <typename T, std::size_t N>
consteval bool can_construct_with_n()
{
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
        return requires { T { (void(Is), anything {})... }; };
    }(std::make_index_sequence<N> {});
}

template <typename T, std::size_t Lo, std::size_t Hi>
consteval std::size_t binary_search_count()
{
    if constexpr (Lo == Hi)
        return Lo;
    else
    {
        constexpr std::size_t mid = Lo + (Hi - Lo + 1) / 2;
        if constexpr (can_construct_with_n<T, mid>())
            return binary_search_count<T, mid, Hi>();
        else
            return binary_search_count<T, Lo, mid - 1>();
    }
}

template <typename T, std::size_t Cap = 31>
consteval std::size_t count_members_impl()
{
    static_assert(can_construct_with_n<T, Cap + 1>() == false,
        "type has more than Cap members; unsupported by SPLIT_FIELDS");
    return binary_search_count<T, 0, Cap>();
}
```

`anything::operator T()` keeps `cpp_utils`'s existing bounded-array guard
(`requires(!std::is_bounded_array_v<std::decay_t<T>>)`). This closes all three drift
points at once: O(log N) counting, no dead branches, hard error instead of silent
truncation past 31, portable to structs with C-array members.

`SPLIT_FIELDS`/`SPLIT_FIELDS_FW_DECL` macros are unchanged (still hand-unrolled 1..31 —
raising the cap is explicitly out of scope).

### 2. Buffer view: `std::span`

Replace `serde::details::_pointer_to_memory`'s duck-typed dispatch (`std::data()`, then
`.data()`, then raw pointer, else `throw std::runtime_error(...)` **at runtime**) with
`std::span<const std::byte>` as the single canonical buffer view used throughout
`deserialize`/`serialize`/`runtime_size`. Anything span-constructible (containers,
arrays, pointer+size) converts implicitly; an unsupported input type becomes a compile
error at the call site instead of a runtime throw for what is fundamentally a
compile-time type mismatch.

### 3. Field-wrapper taxonomy (`serde/special_fields.hpp`)

Existing types (`static_array<T,N>`, `dynamic_array<ID,T>` — element-count sized,
`padding_bytes_t<size,value>`) are unchanged; their behavior and tests keep working as-is.
Three new wrapper types generalize CDFpp's needs:

| New type | Generalizes | Semantics |
|---|---|---|
| `dynamic_array_bytes<ID, T>` | `table_field<T, index>` | Same shape as `dynamic_array`, but its size callback returns a **byte** length instead of an element count; element count = bytes / `sizeof(T)` for fixed-size `T`. |
| `bounded_string<N>` | `string_field<N>` | Reads a `std::string` up to a null terminator within N bytes; writes the string bytes zero-padded to N bytes. |
| `unused<T>` | `unused_field<T>` | Wraps any other field type (fixed or dynamic size). Consumes/produces the same bytes as `T` would, but the decoded value is discarded on load (not stored) and zero-filled on save. |

Each wrapper type opts out of default splitting via the existing nested tag-type
convention (`do_not_split`, `dyn_size_field_tag` where relevant) — no new opt-out
mechanism is introduced; this convention already matches the codebase's "data over code"
style and works fine for the new types.

### 4. Context threading

`deserialize`, `serialize`, and `runtime_size` all take an optional `context` parameter
(defaults to an empty tag type), threaded unchanged through every recursive call into
nested composites. A dynamic field's size is resolved by trying
`parent.field_size(field, context)` first, falling back to `parent.field_size(field)` — so
CDFpp's pattern of a field's size depending on a *sibling record* (e.g. `rVDR_t`'s
`DimVarys` sized by `gdr.rNumDims`, which lives on the enclosing `GDR`, not on `rVDR_t`
itself) is expressible by putting `rNumDims` on the context object passed down from the
top-level call — no change required for `cpp_utils`'s own existing single-argument
`field_size` usage.

### 5. Serialization (`serde/serialization.hpp`, new)

`serialize(value, sink, offset, context)` / `save_field(...)` / `save_fields(...)` mirror
`deserialization.hpp`'s structure field-for-field: same `SPLIT_FIELDS`-driven per-field
dispatch by concept, same recursion into composite fields, same context threading. Each
new field-wrapper type's load and save overloads live together (in `special_fields.hpp`
next to the type, or immediately alongside each other in the load/save headers) rather
than being split across files by operation — matches "locality of reasoning": you should
be able to see how a field type is read *and* written without chasing across the codebase.

Write target is a `byte_sink` **concept**, not a virtual base class — `resize`/`data`/
`size`-shaped, satisfied by `std::vector<uint8_t>`, `std::string`, or (later) a custom
sink backed by `io/memory_mapped_file.hpp`. Consistent with the concept-heavy style
`types/concepts.hpp` already uses elsewhere in this codebase (`container`,
`sequence_container`, etc.) — no vtable, stays header-only.

### 6. Runtime size (`serde/serde.hpp`)

`runtime_size<T>(value, context)`: a `SPLIT_FIELDS`-driven walk parallel to the existing
`consteval composite_size` (which only handles compile-time-constant-size types), but
evaluated at runtime and reusing the same `field_size` resolution as load/save. This is
what lets a future CDFpp cutover precompute a record's `record_size` header field before
writing it (CDFpp's current `update_size` pattern), without ever writing real payload
bytes twice to measure them.

### 7. Host endianness via `<bit>`

Detect host endianness with `std::endian::native == std::endian::little` (C++20,
`constexpr`, from `<bit>`) instead of the `CPP_UTILS_BIG_ENDIAN`/`CPP_UTILS_LITTLE_ENDIAN`
macro pair Meson currently bakes into `config.h` via `target_machine.endian()` — removes a
generated-file dependency for something the standard library now provides directly. The
hand-written `__builtin_bswap*` / `_byteswap_*` byte-swap implementation in
`endianness.hpp` is **unchanged** — not replaced with `std::byteswap`, which is C++23 (see
Constraints).

## Data flow summary

```
deserialize<T>(span, offset, context)
  -> SPLIT_FIELDS destructures T via structured bindings
  -> load_fields walks each field
       -> load_field overload resolved by concept
          (fundamental / enum / composite / static_array / dynamic_array /
           dynamic_array_bytes / bounded_string / unused / padding)
       -> composite fields recurse into deserialize with the same context

serialize(value, sink, offset, context)      // mirrors the above, writing instead of reading
runtime_size<T>(value, context)              // mirrors the above, summing byte lengths only
```

## Testing

New Catch2 tests in `tests/serde/`, written first per this project's TDD workflow rule
(must fail before the corresponding implementation lands), using synthetic structs
(not copied from CDFpp) that exercise:

- Nested composites, per-composite big-endian tag (existing coverage, must keep passing).
- `dynamic_array_bytes` with and without extra context.
- `bounded_string` (short string, string exactly filling N, embedded-null edge case).
- `unused<T>` wrapping both a fixed-size and a dynamic-size inner field.
- Save → load round-trips for every field-wrapper type, including nested composites.
- `runtime_size` matching the actual byte length produced by `serialize`.
- `count_members` edge cases: struct with exactly 31 members (compiles), 32+ members
  (hard compile error with a clear message), struct with a C-array member (compiles and
  counts correctly).

## Open items for the implementation plan

- Exact header split: whether `serialization.hpp` needs its own `special_fields`-adjacent
  save-side declarations file or can share `special_fields.hpp` directly.
- Naming bikeshed: `dynamic_array_bytes` / `bounded_string` / `unused` are working names,
  not final.
- Whether `byte_sink` needs a `reserve`-like hook for pre-sizing via `runtime_size` before
  a `serialize` call, to avoid repeated reallocation on a growing `std::vector` sink.
