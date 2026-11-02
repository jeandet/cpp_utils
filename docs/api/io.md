# io

Three byte-buffer types: `buffer_view`, a non-owning view over memory you already have;
`memory_mapped_file`, a file-backed buffer opened via `mmap`/`MapViewOfFile`; and `owned_buffer`,
an owning buffer backed by `no_init_vector`. All satisfy the same `types::concepts::random_access_buffer`
shape (`read`/`view`/`size`/`is_valid`) and `std::ranges::contiguous_range`, so they're
interchangeable: any can be passed directly to `serde::deserialize`, and code written against one
works unmodified against the others.

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

## owned_buffer

```cpp
#include <io/owned_buffer.hpp>
```

Owning counterpart to `buffer_view`: holds a `no_init_vector<char>` by value and exposes the same
`random_access_buffer` shape (`read`/`view`/`size`/`is_valid`) and contiguous-range interface.
Use it for buffers whose lifetime must outlive their producer — e.g. a freshly-decompressed payload
that has no natural home.

```cpp
struct owned_buffer
{
    owned_buffer() = default;
    explicit owned_buffer(containers::no_init_vector<char>&& storage);
};
```

- `owned_buffer()` — default-constructed, wraps an empty vector; `is_valid()` returns `false`.
- `explicit owned_buffer(containers::no_init_vector<char>&& storage)` — takes ownership of
  `storage`. Any code holding an `owned_buffer` owns the underlying allocation.

```cpp
cpp_utils::containers::no_init_vector<char> data(5);
std::memcpy(data.data(), "hello", 5);

owned_buffer b { std::move(data) };

b.is_valid();                          // true
b.size();                              // 5
std::string(b.data(), b.size());       // "hello"
```

### Methods

- `void read(char* dest, std::size_t offset, std::size_t size) const` — copies `size` bytes
  starting at `offset` into `dest` (`std::memcpy` under the hood; no bounds checking).
- `auto view(std::size_t offset) const` — returns a zero-copy `const char*` pointing at
  `offset` within the buffer.
- `auto data() const` / `std::size_t size() const` — pointer to the start of the buffer, and
  its length in bytes.
- `bool is_valid() const` — `true` iff the backing vector has a non-null data pointer. A
  default-constructed `owned_buffer` is invalid.
- `begin()` / `end()` — contiguous-range iterators (`const char*`), enabling
  `std::ranges::contiguous_range<owned_buffer>`.

```cpp
char dest[3] {};
b.read(dest, 1, 3);
std::string(dest, 3);      // "ell"

*b.view(4);                // 'o'

owned_buffer empty;
empty.is_valid();          // false
```

### With serde

```cpp
#include <io/owned_buffer.hpp>
#include <serde/serde.hpp>

struct wire_pair
{
    uint8_t a;
    uint8_t b;
};

cpp_utils::containers::no_init_vector<char> data(2);
std::memcpy(data.data(), "\x05\x09", 2);

auto value = cpp_utils::serde::deserialize<wire_pair>(
    owned_buffer { std::move(data) }
);
// value.a == 5, value.b == 9
```

## sequential_writer

```cpp
#include <io/sequential_writer.hpp>
```

The write-side counterpart to the read-side `buffer_view`/`owned_buffer`/`memory_mapped_file` types.
Two types satisfy the `sequential_writer` concept: `vector_writer`, which appends into an in-memory
resizable container; and `file_writer`, which streams bytes directly to disk without materializing
the output in memory.

### sequential_writer concept

```cpp
template <typename T>
concept sequential_writer = requires(T t, const char* p, std::size_t n, char v) {
    { t.write(p, n) } -> std::convertible_to<std::size_t>;
    { t.fill(v, n) } -> std::convertible_to<std::size_t>;
    { t.offset() } -> std::convertible_to<std::size_t>;
};
```

Any type satisfying this concept can be passed to `serde::serialize` (in Task 3 and beyond).
All three methods return the new cursor position after the operation.

## vector_writer

Appends into any in-memory, resizable byte container (a `std::vector<char>`, a `no_init_vector<char>`,
or a `cpp_utils::serde dynamic_array_bytes` field) that grows as bytes are written. Non-owning:
the container's lifetime is the caller's responsibility.

```cpp
template <typename Container>
struct vector_writer
{
    explicit vector_writer(Container& data);

    std::size_t write(const char* data_ptr, std::size_t count);
    std::size_t fill(char v, std::size_t count);
    [[nodiscard]] std::size_t offset() const noexcept;
};
```

- `explicit vector_writer(Container& data)` — wraps a reference to `data`. CTAD is enabled via a
  deduction guide, so `vector_writer { my_vector }` deduces the container type automatically.
- `write(data_ptr, count)` — appends `count` bytes starting at `data_ptr` and returns the new cursor
  position.
- `fill(v, count)` — appends `count` copies of byte `v` and returns the new cursor position.
- `offset()` — returns the current cursor position (bytes written so far).

```cpp
std::vector<char> data;
cpp_utils::io::vector_writer writer { data };

writer.write("hello", 5);    // returns 5
writer.fill(' ', 1);         // returns 6
writer.offset();             // 6

std::string(data.data(), data.size());  // "hello "
```

## file_writer

Streams bytes straight to an `fstream` without materializing the output in memory first — needed
for bounded-memory saves of potentially gigabyte-scale files.

```cpp
struct file_writer
{
    explicit file_writer(const std::string& fname);
    ~file_writer();

    [[nodiscard]] bool is_open() const noexcept;
    std::size_t write(const char* data_ptr, std::size_t count);
    std::size_t fill(char v, std::size_t count);
    [[nodiscard]] std::size_t offset() const noexcept;
};
```

- `explicit file_writer(const std::string& fname)` — opens `fname` for writing in binary mode,
  truncating any existing content. The file is closed automatically in the destructor.
- `write(data_ptr, count)` — writes `count` bytes starting at `data_ptr` directly to disk and
  returns the new cursor position.
- `fill(v, count)` — writes `count` copies of byte `v` directly to disk and returns the new cursor
  position.
- `offset()` — returns the total bytes written so far (not the file position on disk, which may
  differ due to buffering).
- `is_open()` — returns `true` iff the file was successfully opened.

```cpp
cpp_utils::io::file_writer writer { "output.bin" };
if (!writer.is_open())
{
    // handle error
}

writer.write("hello", 5);
writer.fill(' ', 1);
writer.offset();             // 6

// file closed automatically when writer is destroyed
```

## range_reader

```cpp
#include <io/range_reader.hpp>
```

A source of bytes fetched by ranges, such as an object on S3. It is the transport that
`remote_buffer` reads through.

```cpp
struct byte_range
{
    std::size_t offset;
    std::size_t size;
};

template <typename R>
concept range_reader = std::copy_constructible<R> && requires(const R& r,
    std::span<const byte_range> ranges, std::span<char* const> destinations) {
    { r.size() } -> std::convertible_to<std::size_t>;
    r.read_ranges(ranges, destinations);
};
```

- `size()` — the total size of the source, in bytes.
- `read_ranges(ranges, destinations)` — fills `destinations[i]` with exactly the bytes of
  `ranges[i]`, or throws. A batch never succeeds partially. Batching lets a transport fetch
  the ranges concurrently; it does not merge them.

## local_range_reader

```cpp
#include <io/local_range_reader.hpp>
```

A `range_reader` over a local file that logs every call. It is meant for tests and for
measuring what a remote reader would be asked for. Copies share the file and the log, so a
copy handed to another object still reports to the original.

- `local_range_reader(path)` — maps the file.
- `batches()` — a snapshot of every `read_ranges` call, one vector of ranges per call.
- `bytes_served()` — the total bytes handed out.
- `caller_threads()` — the thread id of each call, in call order.
- A range past the end throws `std::out_of_range`.

```cpp
cpp_utils::io::local_range_reader reader { "data.bin" };
const cpp_utils::io::byte_range ranges[] { { 0, 4 }, { 8, 4 } };
char a[4], b[4];
char* const destinations[] { a, b };
reader.read_ranges(ranges, destinations);
reader.batches().size();     // 1
reader.bytes_served();       // 8
```

## reserved_region

```cpp
#include <io/reserved_region.hpp>
```

A region of address space the size of a file, whose pages cannot be read until they are
written and sealed. Reading a page that was never sealed crashes, instead of returning zeros.

```cpp
enum class region_backing { memory, file };

class reserved_region
{
public:
    explicit reserved_region(std::size_t size, region_backing backing = region_backing::memory,
        const std::filesystem::path& temp_dir = {});
    char* data() const;
    std::size_t size() const;
    char* make_writable(std::size_t first_page, std::size_t page_count);
    void seal(std::size_t first_page, std::size_t page_count);
    void make_no_access(std::size_t first_page, std::size_t page_count);
};

std::size_t page_size();
```

- `memory` backing reserves anonymous memory. `file` backing maps a temporary file that is
  deleted at once, so the OS can write pages to disk and drop them from RAM. With an empty
  `temp_dir`, the file goes in the user cache directory (`$XDG_CACHE_HOME` or `~/.cache`,
  `~/Library/Caches`, `%LOCALAPPDATA%`). Pick a real disk: on tmpfs a file is just RAM.
- `make_writable` opens pages for writing and returns a pointer to the first one; `seal` makes
  them read-only; `make_no_access` closes them again. A failed protection change throws
  `std::system_error`.
- A size of 0 maps nothing: `data()` is `nullptr`.
- Moving is allowed; copying is not.

```cpp
cpp_utils::io::reserved_region region { 3 * cpp_utils::io::page_size() + 7 };
std::memcpy(region.make_writable(1, 1), "hello", 5);
region.seal(1, 1);
region.data()[cpp_utils::io::page_size()]; // 'h'
region.data()[0];                         // crashes: page 0 was never sealed
```

## remote_buffer

```cpp
#include <io/remote_buffer.hpp>
```

A lazily fetched, read-in-place copy of a `range_reader`'s bytes. It is meant for remote
files (S3...), where every request costs time and money. `ensure` fetches what is missing and
widens it to the read-ahead window, all in one `read_ranges` batch. Each byte is fetched at
most once. Bytes that were never fetched can't be read: `check_present` throws, and a raw read
through `data()` hits a no-access page.

```cpp
struct remote_buffer_options
{
    std::size_t read_ahead = 256 * 1024;
    std::size_t tail_prefetch = 256 * 1024;
    region_backing backing = region_backing::memory;
    std::filesystem::path temp_dir {};
    std::size_t download_whole_below = 0;
    static remote_buffer_options from_env(std::string_view prefix);
    static remote_buffer_options from_env(std::string_view prefix, remote_buffer_options defaults);
};

template <range_reader R>
class remote_buffer
{
public:
    explicit remote_buffer(R reader, remote_buffer_options options = {});
    void ensure(std::span<const byte_range> ranges) const;
    void ensure(std::size_t offset, std::size_t size) const;
    void check_present(std::size_t offset, std::size_t size) const;
    void read(char* dest, std::size_t offset, std::size_t size) const;
    const char* data() const;
    const char* begin() const;
    const char* end() const;
    std::size_t size() const;
    bool is_valid() const;
    const R& reader() const;
};
```

- `read_ahead`: each fetch is widened to aligned windows of this size, rounded up to whole
  pages.
- `tail_prefetch`: the first `ensure` also fetches this many bytes from the end of the file,
  in the same batch. Formats that write metadata last (CDF) need them early.
- `download_whole_below`: if the file is smaller than this, the first `ensure` fetches all of
  it. 0 turns it off.
- `from_env("PREFIX_")` reads `PREFIX_READ_AHEAD`, `PREFIX_TAIL_PREFETCH`, `PREFIX_BACKING`
  (`memory` or `file`), `PREFIX_TEMP_DIR` and `PREFIX_DOWNLOAD_WHOLE_BELOW`. A bad value
  throws `std::invalid_argument`.
- A range outside the file throws `std::runtime_error`. That check can't overflow, even on
  corrupt offsets.
- If the reader throws, `ensure` rethrows and the batch's pages stay unreadable. A later call
  retries them.
- `ensure` is thread-safe. The reader is never called while the cache lock is held. Threads
  that need pages another thread is fetching wait for that fetch, rather than fetching again.

```cpp
cpp_utils::io::local_range_reader reader { "data.bin" };
cpp_utils::io::remote_buffer buffer { reader };
buffer.ensure(10, 4);        // one batch: [0, 256 KiB) + the last 256 KiB
buffer.data()[12];           // read in place
buffer.ensure(20, 4);        // already there: no fetch
```
