# io

Two byte-buffer types: `buffer_view`, a non-owning view over memory you already have, and
`memory_mapped_file`, a file-backed buffer opened via `mmap`/`MapViewOfFile`. Both satisfy the
same `types::concepts::random_access_buffer` shape (`read`/`view`/`size`/`is_valid`) and
`std::ranges::contiguous_range`, so they're interchangeable: either can be passed directly to
`serde::deserialize`, and code written against one works unmodified against the other.

## buffer_view

```cpp
#include <io/buffer_view.hpp>
```

Read-only, non-owning view over an in-memory byte buffer (`std::vector<char>`, `std::array`,
another `std::span`, ...). Ownership of the backing storage stays with the caller, exactly like
`std::span`/`std::string_view` — the buffer must outlive the `buffer_view`.

```cpp
struct buffer_view
{
    std::span<const char> bytes;

    buffer_view() = default;
    explicit buffer_view(std::span<const char> bytes);
};
```

- `buffer_view()` — default-constructed, wraps a null/empty span; `is_valid()` returns `false`.
- `explicit buffer_view(std::span<const char> bytes)` — wraps `bytes`. Anything
  span-constructible (a `std::vector<char>`, `std::array<char, N>`, etc.) converts implicitly at
  the call site.

```cpp
std::vector<char> data { 'h', 'e', 'l', 'l', 'o' };
buffer_view v { data };

v.is_valid();                          // true
v.size();                              // 5
std::string(v.data(), v.size());       // "hello"
```

### Methods

- `void read(char* dest, std::size_t offset, std::size_t size) const` — copies `size` bytes
  starting at `offset` into `dest` (`std::memcpy` under the hood; no bounds checking).
- `auto view(std::size_t offset) const` — returns a zero-copy `const char*` pointing at
  `offset` within the buffer.
- `auto data() const` / `std::size_t size() const` — pointer to the start of the buffer, and
  its length in bytes.
- `bool is_valid() const` — `true` iff the wrapped span has a non-null data pointer. A
  default-constructed `buffer_view` is invalid.
- `begin()` / `end()` — contiguous-range iterators (`const char*`), enabling
  `std::ranges::contiguous_range<buffer_view>`.

```cpp
char dest[3] {};
v.read(dest, 1, 3);
std::string(dest, 3);      // "ell"

*v.view(4);                 // 'o'

buffer_view empty;
empty.is_valid();           // false
```

### With serde

```cpp
#include <io/buffer_view.hpp>
#include <serde/serde.hpp>

struct wire_pair
{
    uint8_t a;
    uint8_t b;
};

std::vector<char> data { static_cast<char>(5), static_cast<char>(9) };
auto value = cpp_utils::serde::deserialize<wire_pair>(buffer_view { data });
// value.a == 5, value.b == 9
```

## memory_mapped_file

```cpp
#include <io/memory_mapped_file.hpp>
```

Maps a file into memory read-only (`mmap` on POSIX, `MapViewOfFile` on Windows) and exposes it
as a contiguous byte buffer. The mapping is torn down in the destructor (`munmap`/
`UnmapViewOfFile` plus the underlying file handle), so a `memory_mapped_file` follows normal
RAII scoping — no explicit close call.

```cpp
struct memory_mapped_file
{
    memory_mapped_file(const std::string& path);
    ~memory_mapped_file();
};
```

- `memory_mapped_file(const std::string& path)` — opens and maps `path`. If the path doesn't
  exist, the file is empty, `open`/`CreateFile` fails, or the `mmap`/`MapViewOfFile` call itself
  fails, construction still succeeds as an object but leaves it in an invalid state — check
  `is_valid()` (and/or `size()`) before using it, rather than expecting an exception. (The
  constructor calls `std::filesystem::exists`/`file_size` internally, which can still throw
  `std::filesystem::filesystem_error` on OS-level errors other than "missing file", e.g.
  permission denied on a parent directory.)

```cpp
memory_mapped_file f { "data.bin" };
if (!f.is_valid())
{
    // path missing, empty file, or the OS-level mapping call failed
}
```

```cpp
memory_mapped_file f { "/nonexistent/path/that/should/not/exist.bin" };
f.is_valid();   // false
f.size();       // 0
```

### Methods

- `void read(char* dest, std::size_t offset, std::size_t size) const` — copies `size` bytes
  starting at `offset` into `dest` (`std::memcpy`; no bounds checking).
- `auto read(std::size_t offset, std::size_t) const` — returns a `char*` at `offset` (the
  trailing size argument is accepted for interface symmetry with the copying overload but
  unused).
- `template <std::size_t size> auto read(std::size_t offset) const` — same as the two-argument
  `read` above; the size template parameter is currently unused too.
- `auto view(std::size_t offset) const` — zero-copy `char*` pointing at `offset` within the
  mapped file.
- `template <std::size_t size> auto view(std::size_t offset) const` — same as `view(offset)`
  above; the size template parameter is currently unused.
- `auto size() const` — mapped file size in bytes (`0` if invalid).
- `auto data() const` — equivalent to `view(0)`, pointer to the start of the mapping.
- `bool is_valid() const` — `true` iff the file was opened and successfully mapped.
- `template <typename T = std::byte> std::span<const T> as_span() const` — the whole mapping
  reinterpreted as `std::span<const T>`, `f_size / sizeof(T)` elements long. Defaults to
  `std::byte`; pass e.g. `char` for a text view.
- `begin()` / `end()` — contiguous-range iterators (`char*`), enabling
  `std::ranges::contiguous_range<memory_mapped_file>`.

```cpp
memory_mapped_file f { "data.bin" };

f.is_valid();                          // true
f.size();                              // e.g. 11
std::string(f.data(), f.size());       // file contents

auto bytes = f.as_span<std::byte>();   // std::span<const std::byte>
auto chars = f.as_span<char>();        // std::span<const char>
```

### With serde

`memory_mapped_file` itself satisfies `random_access_buffer`, so it can be handed to
`serde::deserialize` directly — no `as_span()` needed — or via `as_span()` if a `std::span` is
preferred:

```cpp
#include <io/memory_mapped_file.hpp>
#include <serde/serde.hpp>

struct wire_pair
{
    uint8_t a;
    uint8_t b;
};

memory_mapped_file f { "data.bin" };

auto value = cpp_utils::serde::deserialize<wire_pair>(f);                  // direct
auto same  = cpp_utils::serde::deserialize<wire_pair>(f.as_span<std::byte>()); // via span
```
