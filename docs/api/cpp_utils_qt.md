# cpp_utils_qt

Qt6 integration for `cpp_utils`. Kept separate from the core headers so consumers who don't use
Qt never pull in Qt headers.

**Opt-in build requirement:** this module is only built when the Meson option `-Dqt=true` is set
(`meson setup build -Dqt=true`), which links Qt6 Core/Widgets/Gui. With the default configuration
none of the headers below are compiled, and their symbols don't exist.

`cpp_utils_qt` doesn't introduce new concepts of its own — it specializes/extends the generic
machinery from [`types`](types.md) (smart-pointer detection, string conversion) and
[`trees`](trees.md) (tree traversal traits) so it works with Qt types. Read those pages for the
concepts being extended; this page documents only the Qt-specific additions.

## `cpp_utils_qt/cpp_utils_qt.hpp`

Umbrella header — includes all three headers documented below. Use it to pull in every Qt
specialization at once.

```cpp
#include <cpp_utils_qt/cpp_utils_qt.hpp>
```

## `cpp_utils_qt/trees/traits.hpp`

Makes `QTreeWidgetItem` usable directly as a node type with the `trees::` algorithms (e.g.
`print_tree`, `for_each`), by specializing the three customization points those algorithms are
templated on:

- `child<QTreeWidgetItem&>(QTreeWidgetItem& node, int index)` → `QTreeWidgetItem&`, returns
  `*node.child(index)`
- `children_count<QTreeWidgetItem&>(QTreeWidgetItem& node)` → `std::size_t`, returns
  `node.childCount()`
- `name<QTreeWidgetItem&>(QTreeWidgetItem& node)` → returns `node.text(0)` (a `QString`)

Matching `QTreeWidgetItem&&` overloads are provided for all three, so both lvalue and rvalue
`QTreeWidgetItem` nodes work.

The whole header is guarded by `#if __has_include(<QTreeWidgetItem>)`: if `QTreeWidgetItem`
(QtWidgets) isn't available it compiles to nothing rather than failing.

Usage — build a `QTreeWidgetItem` hierarchy and hand it straight to a `trees::` algorithm, no
adapter code required (adapted from `tests/trees/test_print_qt.cpp`):

```cpp
#include <QString>
#include <QTreeWidgetItem>
#include <cpp_utils_qt/cpp_utils_qt.hpp>
#include <trees/algorithms.hpp>

using namespace cpp_utils::trees;

auto* root = new QTreeWidgetItem {};
root->setText(0, "root");
auto* child = new QTreeWidgetItem {};
child->setText(0, "node 0");
root->addChild(child);

std::stringstream ss;
print_tree(root, ss);
// ss.str() == "root\n├── node 0\n"
```

## `cpp_utils_qt/types/converters.hpp`

Adds `QString` support to the generic `to_string<str_t>()` conversion, and a
`QString -> std::string` overload of `std::to_string`.

### `cpp_utils::types::strings::to_string<QString>(const T&)`

Enabled only when the explicit template argument is `QString`; formats `object` through
`QString::arg`.

```cpp
#include <cpp_utils_qt/types/converters.hpp>
#include <QString>

QString s = cpp_utils::types::strings::to_string<QString>(42);
// s == "42"
```

### `std::to_string(const QString&)`

Adds an overload of `std::to_string` for `QString`, returning `str.toStdString()`. This puts a
function into namespace `std` for a third-party (Qt) type — non-standard, but works with all
mainstream compilers.

```cpp
#include <cpp_utils_qt/types/converters.hpp>
#include <QString>

QString q = "hello";
std::string s = std::to_string(q);
// s == "hello"
```

## `cpp_utils_qt/types/pointers.hpp`

Adds detection traits for Qt's smart-pointer types (list taken from
[the Qt wiki](https://wiki.qt.io/Smart_Pointers)) and wires them into the generic
`types::pointers::is_smart_ptr` trait from [`types`](types.md).

| Trait | Detects |
|---|---|
| `is_QSharedDataPointer<T>` / `_v` | `QSharedDataPointer<U>` |
| `is_QExplicitlySharedDataPointer<T>` / `_v` | `QExplicitlySharedDataPointer<U>` |
| `is_QSharedPointer<T>` / `_v` | `QSharedPointer<U>` |
| `is_QWeakPointer<T>` / `_v` | `QWeakPointer<U>` |
| `is_QScopedPointer<T>` / `_v` | `QScopedPointer<U>` |
| `is_QScopedArrayPointer<T>` / `_v` | `QScopedArrayPointer<U>` |

`is_qt_smart_ptr<T>` / `is_qt_smart_ptr_v<T>` is true if `T` is any of the six above. The header
also specializes `types::pointers::is_smart_ptr<T>` so the generic `is_smart_ptr_v<T>` (see
[`types`](types.md)) is `true` for Qt smart pointers too, without callers needing to know which
specific trait applies.

Usage (adapted from `tests/types/qt_types_detector.cpp`):

```cpp
#include <cpp_utils_qt/cpp_utils_qt.hpp>
#include <QSharedPointer>

using namespace cpp_utils::types::pointers;

static_assert(is_QSharedPointer_v<QSharedPointer<int>>);
static_assert(is_qt_smart_ptr_v<QSharedPointer<int>>);
static_assert(is_smart_ptr_v<QSharedPointer<int>>); // generic trait, now Qt-aware
```
