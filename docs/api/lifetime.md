# lifetime

RAII scope-leaving guards: bind a pointer to a callback that runs automatically when the guard goes out of scope, similar in spirit to `absl::Cleanup` or a hand-rolled `scope_exit`. The whole module is one type alias built on `std::unique_ptr`.

```cpp
#include <lifetime/scope_leaving_guards.hpp>
```

## `scope_leaving_guard<T, callback_t>`

```cpp
namespace cpp_utils::lifetime
{
template <typename T, auto callback_t>
using scope_leaving_guard = std::unique_ptr<T, /* deleter calling callback_t(obj) */>;
}
```

An alias for `std::unique_ptr<T, Deleter>` whose deleter simply calls `callback_t(obj)` on the wrapped `T*` when the guard is destroyed. `T` is the pointee type; `callback_t` is a non-type template parameter — a free function or other compile-time-constant callable — invocable as `callback_t(T*)`.

There is no separate factory function: construct it directly from a `T*`, the same way you'd construct a `std::unique_ptr`.

Because it *is* a `std::unique_ptr` specialization, it inherits the full `unique_ptr` interface: it is move-only (not copyable), and supports `get()`, `reset()`, and `release()`.

```cpp
#include <lifetime/scope_leaving_guards.hpp>

struct Resource
{
    int count = 1;
};

void release_resource(Resource* obj)
{
    obj->count--;
}

using namespace cpp_utils::lifetime;

Resource r;
{
    auto guard = scope_leaving_guard<Resource, release_resource>(&r);
    // r.count == 1 here
} // release_resource(&r) runs here; r.count == 0
```

## Dismissing the guard

To cancel the guard without running its callback, call the inherited `release()` — it hands back the raw pointer and clears the guard's internal pointer, so the deleter has nothing to act on at scope exit:

```cpp
Resource r;
{
    auto guard = scope_leaving_guard<Resource, release_resource>(&r);
    guard.release(); // dismissed: release_resource will NOT run
} // r.count is still 1
```
