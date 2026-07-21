# cpp_utils

[![GH Actions](https://github.com/jeandet/cpp_utils/actions/workflows/CI.yml/badge.svg)](https://github.com/jeandet/cpp_utils/actions/workflows/CI.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A header-only C++20 utility library — this is where [Alexis Jeandet](https://github.com/jeandet)
keeps the general-purpose C++ building blocks reused across his own projects: compile-time
reflection over aggregates, binary serialization, endianness-aware decoding, and a handful of
container, string, and tree helpers. Not tied to any particular file format or domain.

Almost all logic lives in `include/cpp_utils/` — there is no `.cpp` source beyond the test suite.

## Why the serde module works this way

Binary record formats are often just plain structs with a handful of recurring quirks: fixed-size
arrays, variable-length arrays sized by another field, padding, big-endian wire order,
unused/reserved fields, fixed-point scaled values. `cpp_utils` lets you describe those structs as
ordinary C++ aggregates and get (de)serialization for free, without writing per-field boilerplate
or codegen — a compile-time reflection layer walks the struct's fields for you.

## Features

- **Reflection over aggregates** (`reflexion/`, [docs](docs/api/reflexion.md)) — count and
  iterate a plain struct's members at compile time (up to 31 members) using only C++20 language
  features, no macros or codegen.
- **Binary serde** (`serde/`, [docs](docs/api/serde.md)) — symmetric `serialize`/`deserialize`
  for any reflectable struct, with field wrappers for fixed/dynamic arrays, bounded strings,
  padding, unused/reserved bytes, and fixed-point scaled values.
- **Endianness helpers** (`endianness/`, [docs](docs/api/endianness.md)) —
  compile-time-aware byte-swapping decode/encode, per-struct opt-in big-endian wire order.
- **Buffer/IO abstractions** (`io/`, [docs](docs/api/io.md)) — `memory_mapped_file` and
  `buffer_view`, both usable interchangeably as `serde::deserialize` inputs.
- **Containers** (`containers/`, [docs](docs/api/containers.md)) — `no_init_vector` (skips
  value-initialization for perf-sensitive buffers), `nomap` (vector-backed associative
  container), small algorithms (`contains`, `index_of`, `split`, `join`, `broadcast`, ...),
  N-D shape helpers (`flat_index`, `permute_axes`).
- **Trees** (`trees/`, [docs](docs/api/trees.md)) — a generic `tree_node<T>` plus traversal
  (`for_each`, `print_tree`) that works over user-supplied tree-like types.
- **Strings** (`strings/`, [docs](docs/api/strings.md)) — `trim`/`ltrim`/`rtrim`,
  `make_unique_name`, `fuzzy_match`.
- **Lifetime** (`lifetime/`, [docs](docs/api/lifetime.md)) — RAII scope-leaving guards.
- **Concepts & type traits** (`types/`, [docs](docs/api/types.md)) — `random_access_buffer`,
  `container`, `contiguous_sequence_container`, member/method detection idioms, smart-pointer
  helpers, integer helpers, `Visitor<Ts...>`, `dispatch_dtype`.
- **Parallelism** (`threading/`, [docs](docs/api/threading.md), opt-in via `-Dthreading=true`)
  — `parallel_for`/`parallel_for_each`, plus span-based `parallel_chunks_for_each`/
  `_transform`/`_reduce`.
- **Optional Qt support** (`cpp_utils_qt/`, [docs](docs/api/cpp_utils_qt.md), opt-in via
  `-Dqt=true`) — Qt<->native type traits and tree traits, kept out of the core headers so
  non-Qt consumers never pull in Qt.

See [`docs/api/`](docs/api/) for the full API reference — one page per module, documenting every
public symbol with real, verified-accurate usage examples.

## Requirements

- A C++20 compiler (concepts, `std::span`, `std::endian` are used directly — `cpp_std=c++20` is
  hard-pinned in `meson.build`).
- [Meson](https://mesonbuild.com/) >= 1.2.0 and Ninja.
- [Hedley](https://github.com/nemequ/hedley) (portability macros) — pulled automatically via the
  Meson wrap in `subprojects/`.
- [Catch2](https://github.com/catchorg/Catch2) v3 — tests only, pulled via wrap.
- Qt6 (Core/Widgets/Gui) — only if built with `-Dqt=true`.

No network access is required to build if the wrap cache under `subprojects/packagecache` is
present.

## Using it in your project

The simplest way to consume `cpp_utils` is as a Meson wrap. Add
`subprojects/cpp_utils.wrap`:

```ini
[wrap-git]
url = https://github.com/jeandet/cpp_utils
revision = main
depth = 1

[provide]
cpp_utils = cpp_utils_dep
```

then depend on it like any other Meson dependency, falling back to the wrap when it isn't
installed on the system:

```meson
cpp_utils_dep = dependency('cpp_utils',
    fallback : ['cpp_utils', 'cpp_utils_dep'],
    default_options : ['with_tests=false'])

executable('my_app', 'main.cpp', dependencies : [cpp_utils_dep])
```

Or drop `include/cpp_utils/` on your include path directly if you don't use Meson — it has no
build step of its own.

## Quick start

### Reflection

```cpp
#include <reflexion/reflection.hpp>

struct point { float x, y, z; };
static_assert(cpp_utils::reflexion::count_members<point> == 3);
```

### Serialization

Describe the on-wire layout as an ordinary struct, then serialize/deserialize it directly:

```cpp
#include <serde/serde.hpp>

struct packet
{
    using endianness = cpp_utils::endianness::big_endian_t; // default is little-endian
    uint32_t id;
    uint16_t payload_size;
    cpp_utils::serde::dynamic_array_bytes<0, uint16_t> payload;

    // resolves the wire size of `payload` from another field of the same struct
    std::size_t field_size(const decltype(payload)&) const { return payload_size; }
};

auto p = cpp_utils::serde::deserialize<packet>(buffer);

std::vector<uint8_t> sink;
cpp_utils::serde::serialize(p, sink);
```

Other field wrappers available in `serde::special_fields.hpp`:

| Wrapper | Purpose |
|---|---|
| `static_array<T, N>` | fixed-size array field |
| `dynamic_array<ID, T>` | resizable array, size resolved via `field_size()` (element count) |
| `dynamic_array_bytes<ID, T>` | same, but the size callback returns a **byte** length |
| `dynamic_array_until_eof<T>` | consumes the rest of the buffer |
| `bounded_string<N>` | null-terminated string, zero-padded to N bytes on write |
| `scaled<wire_t, real_t, Num, Den>` | fixed-point wire value exposed as a scaled real (e.g. `int16_t` raw -> `double` engineering unit) |
| `unused<T>` | decoded value discarded on load, zero-filled on save |
| `padding_bytes_t<size, value>` | fixed-size skip/fill field |

When a dynamic field's size depends on data outside its immediate parent struct, thread a
`context` object through instead:

```cpp
struct dims_context { int32_t num_dims; };
struct record
{
    cpp_utils::serde::dynamic_array_bytes<0, int32_t> values;
    std::size_t field_size(const decltype(values)&, const dims_context& ctx) const
    {
        return static_cast<std::size_t>(ctx.num_dims) * sizeof(int32_t);
    }
};

auto r = cpp_utils::serde::deserialize<record>(buffer, 0, dims_context { 2 });
```

`serde::runtime_size<T>(value, context)` computes the exact byte length `serialize` would
produce without writing anything — handy for filling in a record's own size header up front.

### Endianness

```cpp
#include <endianness/endianness.hpp>

using cpp_utils::endianness::big_endian_t;
auto v = cpp_utils::endianness::decode<big_endian_t, uint32_t>(ptr); // swaps only if needed
```

### Buffers & memory-mapped files

`memory_mapped_file` and `buffer_view` both satisfy
`types::concepts::random_access_buffer` and `std::ranges::contiguous_range`, so either can be
handed directly to `serde::deserialize`:

```cpp
#include <io/memory_mapped_file.hpp>

cpp_utils::io::memory_mapped_file file { "data.bin" };
auto record = cpp_utils::serde::deserialize<packet>(file);
```

### Containers, trees, strings

```cpp
#include <containers/algorithms.hpp>
#include <trees/algorithms.hpp>
#include <strings/algorithms.hpp>

cpp_utils::containers::contains(my_vector, 42);
cpp_utils::trees::for_each(my_tree, [](auto& node) { /* ... */ });
cpp_utils::strings::trim(s);
```

## Building & testing

```sh
meson setup build
ninja -C build
meson test -C build            # or: ninja -C build test
```

Run a single test (declared as `Test-<name>` in `tests/meson.build`):

```sh
meson test -C build Test-TestDeserialization
./build/tests/TestDeserialization
```

Useful `meson_options.txt` switches:

- `-Dwith_tests=false` — skip building/subdir'ing `tests/`.
- `-Dqt=true` — also build the Qt-dependent test variants (`*_qt.cpp`), linking Qt6
  Core/Widgets/Gui. Off by default.

CI (`.github/workflows/CI.yml`) runs on every push/PR/release:

- default build+test matrix on the latest Linux, Windows, and macOS (Apple Silicon) runners
- a Linux job with `-Dqt=true`, exercising the Qt-dependent test variants
- a Linux ASan+UBSan job (`-Db_sanitize=address,undefined`)
- a Linux job with `-Dcpp_std=c++23`, exercising the opt-in `std::byteswap` path in
  `endianness.hpp` (see below) — the project itself still targets C++20

A separate workflow (`.github/workflows/tests-with-coverage.yml`) builds Linux with
`-Db_coverage=true` and uploads an lcov report as a build artifact.

Formatting follows `.clang-format` (WebKit-based, Allman braces, 4-space indent, 100-column
limit).

## Project layout

```
include/cpp_utils/
├── reflexion/     # compile-time aggregate member counting & field iteration
├── serde/         # binary (de)serialization built on reflexion + endianness
├── endianness/    # byte-swapping decode/encode
├── types/         # concepts, member/method detectors, integer & pointer helpers, Qt traits
├── containers/    # no_init_vector, nomap, container algorithms/traits
├── trees/         # generic tree node + traversal
├── strings/       # small string algorithms
├── lifetime/      # RAII scope-leaving guards
├── io/            # memory_mapped_file, buffer_view
└── cpp_utils_qt/  # optional Qt-specific specializations
tests/             # Catch2 v3 test suite, mirrors the module layout above
```

## License

MIT © Laboratory of Plasma Physics — CNRS. See license headers in individual source files.
