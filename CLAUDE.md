# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`cpp_utils` is a header-only C++20 utility library (MIT license, LPP/CNRS) providing low-level
building blocks used by other Alexis Jeandet projects (e.g. Speasy/SciQLop family): binary
serialization via reflection, endianness-aware decoding, container helpers, string/tree
algorithms, and optional Qt type traits. Almost all logic lives in `include/cpp_utils/`; there is
no `.cpp` source beyond tests.

`docs/api/` has one Markdown page per module (`docs/api/README.md` is the index) documenting every
public symbol with its real signature and a verified usage example — linked from the main
[README](README.md)'s Features section. Prefer reading the relevant page there over re-deriving a
public API from source when the two might disagree; update the page (not just the header) when you
change a public symbol's signature or behavior.

## Build & test

Build system is Meson + Ninja (`meson_version >= 1.2.0`).

```sh
meson setup build
ninja -C build
meson test -C build            # or: ninja -C build test
```

Run a single test (test names are declared in `tests/meson.build` as `Test-<name>`, e.g.
`Test-TestDeserialization`):

```sh
meson test -C build Test-TestDeserialization
# or run the executable directly:
./build/tests/TestDeserialization
```

Useful options (`meson_options.txt`):
- `-Dwith_tests=false` — skip building/subdir'ing `tests/`.
- `-Dqt=true` — also build the Qt-dependent test variants (`*_qt.cpp` files), which link Qt6
  Core/Widgets/Gui. Off by default.
- `-Dthreading=true` — also build `TestParallelFor`, which needs the `bshoshany-thread-pool`
  wrap. Off by default (mirrors `-Dqt=true`'s `disabler()` pattern exactly) so a plain
  `meson setup` never needs network access for a dependency most builds don't use. CI's main
  build-and-test job passes this on all platforms; there's no dedicated `threading` job like
  the Qt one since no extra system package is needed, unlike Qt6.

Dependencies are pulled via Meson subprojects/wraps (`subprojects/*.wrap`, vendored in
`subprojects/packagecache`): `hedley` (portability macros, always required) and `catch2-with-main`
(v3, tests only). No network access is needed if the wrap cache is present. `bshoshany-thread-pool`
is a third wrap, fetched only when `-Dthreading=true`; like Qt, it is deliberately NOT part of
`cpp_utils_dep` — only `threading/parallel_for.hpp` and its test need it, wired directly into that
test's `deps` in `tests/meson.build`, so plain consumers of `cpp_utils_dep` never pull it in.

`include/cpp_utils/meson.build` generates `config.h` from `config.h.in` at configure time — it
defines `CPP_UTILS_VERSION`. `types/concepts.hpp` no longer depends on `config.h`: concepts are
mandatory core-language C++20 (not an optional/probed feature), and `cpp_std=c++20` is
hard-pinned, so the old compile-check + `CPP_UTILS_CONCEPTS_SUPPORTED` gate was dead code and was
removed. Host endianness detection (`endianness.hpp`) similarly no longer depends on `config.h`
— it's computed directly from `std::endian::native` (`<bit>`, C++20).

CI (`.github/workflows/CI.yml`) builds+tests on ubuntu/windows/macos-13/macos-14 via
`meson setup && ninja && ninja test`. `.github/workflows/tests-with-coverage.yml` runs an
additional Linux build with `-Db_coverage=true` and produces an lcov report.

Formatting follows `.clang-format` (WebKit-based, Allman braces, 4-space indent, 100 col limit).

## Architecture

### Module layout

Each subdirectory of `include/cpp_utils/` is an independent domain; headers are added explicitly
to the `headers` list in the root `meson.build` (new public headers must be added there too):

- `reflexion/` — the foundation the rest of the library builds on (see below).
- `serde/` — binary (de)serialization built on top of `reflexion` + `endianness`.
- `endianness/` — compile-time-aware byte-swapping and little/big-endian decode helpers.
- `types/` — concepts (`concepts.hpp`), member/method/type detection idioms
  (`detectors.hpp`), smart pointer helpers, integer helpers, `Visitor<Ts...>` (`visitor.hpp`,
  the variant-overload CTAD idiom), `dispatch_dtype` (`dtype_dispatch.hpp`, runtime
  format-char → compile-time-type dispatch, e.g. for numpy/buffer-protocol interop).
- `containers/` — `no_init_vector` (vector without value-initialization for perf-sensitive
  buffers), `nomap` (vector-backed associative container), container algorithms/traits
  (`algorithms.hpp` also has `flat_size` — shape-product/element-count helper), `nd_shape.hpp`
  (`flat_index`/`permute_axes` — arbitrary-dimension-count N-D flat indexing and axis
  permutation, row-major or column-major).
- `trees/` — a generic tree `tree_node<T>` plus traversal algorithms and trait detection so trees can
  be templated over user-supplied node-like types. `for_each` takes an optional `traversal_order`
  (`pre_order` default, `post_order`, `breadth_first`) selecting visit order at runtime, mirroring
  `containers::array_order`'s default-argument-enum pattern.
- `strings/` — small string algorithms, plus `fuzzy_match.hpp` (character/container-generic
  subsequence fuzzy-match scoring with word-boundary/camelCase bonuses — templated on `CharT`,
  ASCII-range case-folding only, not a full Unicode case-fold).
- `threading/` — `parallel_for`/`parallel_for_each` over a lazily-initialized, hardware-
  concurrency-sized `BS::light_thread_pool` singleton. Needs the `bshoshany-thread-pool`
  dependency explicitly (see Build & test above) — not part of `cpp_utils_dep`. Also
  `parallel_chunks_for_each`/`parallel_chunks_transform`/`parallel_chunks_reduce`
  (`parallel_chunks.hpp`) — chunk-based dispatch built on `BS::thread_pool`'s own
  `detach_blocks`/`submit_blocks` block-splitting rather than hand-rolled chunk math, each
  skipping the pool for a single serial call when the input is smaller than a caller-supplied
  `min_chunk_size * chunk_count` threshold. Distinct from `parallel_for`: chunked functions
  hand each chunk a contiguous `std::span` of real data, not a bare index.
- `lifetime/` — RAII scope-leaving guards (run code on scope exit).
- `io/` — `memory_mapped_file` (file-backed) and `buffer_view` (non-owning view over an
  in-memory buffer). Both satisfy `types::concepts::random_access_buffer`
  (`read`/`view`/`size`/`is_valid`) and `std::ranges::contiguous_range`, so either can be
  passed directly to `serde::deserialize` or used interchangeably wherever a byte buffer is
  expected.
- `cpp_utils_qt/` — Qt-specific specializations (pointer types, tree traits, converters). Kept
  separate from the core headers so consumers who don't use Qt never pull in Qt headers. Same
  reasoning as `threading/`'s dependency isolation above.

### Reflection (`reflexion/reflection.hpp`)

This is the load-bearing trick the rest of the library depends on. C++20 aggregates can be
destructured with structured bindings, but the binding count must be known at compile time. The
`details::MemberCounter` template (an `anything`-convertible probe object plus a
`consteval` binary search over `[0, 31]`) counts an aggregate's members at compile time
without macros or codegen. A type needing more than 31 members is a hard compile error
from `count_members` itself, not just from `SPLIT_FIELDS`.

The `SPLIT_FIELDS_FW_DECL`/`SPLIT_FIELDS` macro pair generates, for member counts 1..31, a
`switch`-like `if constexpr` cascade that structured-binds the aggregate into `_0.._N` and forwards
them positionally to a callback — effectively "for each field of this struct, do X" for arbitrary
aggregates. This is how `reflexion::composite_size`, `composite_have_const_size`, and
`serde::deserialize` walk struct fields without per-type boilerplate. **31 members is a hard
ceiling** (`static_assert(count <= 31)`); extending it means growing every `SPLIT_FIELDS` cascade.

Types opt out of this field-splitting behavior with tag types rather than specialization:
- nested `using do_not_split = reflexion::do_not_split_t;` makes `can_split_v<T>` false, so the
  type is treated as an atomic field (used by `static_array`, `dynamic_array`,
  `padding_bytes_t`).
- nested `using dyn_size_field_tag = reflexion::dyn_size_field_tag_t;` marks a field as not
  having a compile-time-known size (used by `dynamic_array`), which propagates through
  `composite_have_const_size`/`composite_size` (both `consteval`, so a struct's binary size is
  computed at compile time when every field is constant-size).

This tag-type convention (nested type alias checked with `requires { typename T::tag_name; }`) is
the idiomatic way this codebase adds cross-cutting metadata to a type — follow it rather than
introducing a new trait mechanism.

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

**When a new field-wrapper type has zero real data members** (only `static constexpr`/`using` 
members — no plain data field), it will break `reflexion::count_members` if placed anywhere but 
the last member of its enclosing struct: a 0-member aggregate can't absorb the single 
`anything`-convertible slot the counting technique probes it with. Give it a private dummy 
data member (e.g. `char _reserved = 0;`) to force it to be treated as one opaque unit, the 
same way `static_array`/`dynamic_array` already are via their own private storage. Types with 
at least one real member, or that inherit non-aggregate storage from a base class, don't need 
this.

### Endianness (`endianness/endianness.hpp`)

Host endianness (`host_is_big_endian`/`host_is_little_endian`) is computed via
`std::endian::native` (`<bit>`, C++20) — no longer a Meson-generated `config.h` macro.
`decode<src_endianness_t, T>(ptr)` byte-swaps only when `src_endianness_t` differs from
`host_endianness_t`, and is a no-op for single-byte types; `encode<dst_endianness_t, T>(value,
output)` is the write-side counterpart used by `serde::serialize`. `decode_v`/`encode_v` are the
bulk/array variants used by `serde` for array fields. Byte-swapping itself defaults to
`__builtin_bswap*`/`_byteswap_*`, since this project hard-pins `cpp_std=c++20` (the codebase
must build across a wide toolchain matrix, including `cibuildwheel`-driven wheel builds for
downstream consumers — MSVC has no stable non-preview `/std:c++23` yet, and Meson itself doesn't
map `cpp_std=c++23` to an MSVC flag as of mesonbuild/meson#15981). `details::bswap` opportunistically
switches to `std::byteswap` when `__cpp_lib_byteswap` is defined (i.e. when a consumer builds this
header under `-std=c++23`+ with a standard library that implements it) — dead code under the
project's own C++20 build, exercised by CI's `cpp23` job so it doesn't bit-rot.

### Tests

`tests/` mirrors the `include/cpp_utils/` module layout (one directory per domain) and uses
Catch2 v3 BDD-style macros (`GIVEN`/`WHEN`/`THEN`) in the serde tests; other domains mostly use
plain `TEST_CASE`/`REQUIRE`. Qt-dependent tests live alongside their non-Qt counterparts with a
`_qt` suffix (e.g. `trees/test_print_qt.cpp`) and are only registered/built when `-Dqt=true`.
