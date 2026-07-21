# trees

A generic tree: an owning node type (`tree_node<string_type>`) you can build trees with directly,
plus traversal (`for_each`) and printing (`print_tree`) algorithms that don't require that type —
they work over any type exposing the small `child`/`children_count`/`name` protocol described
below.

```cpp
#include <trees/node.hpp>
#include <trees/algorithms.hpp>
#include <trees/traits.hpp>
```

## `tree_node<string_type>`

```cpp
template <typename string_type = std::string>
struct tree_node
{
    string_type name;
    tree_node* parent;
    std::vector<std::unique_ptr<tree_node<string_type>>> children;

    tree_node(const string_type& name, tree_node* parent = nullptr);

    iterator_t begin();       // over children
    iterator_t end();
    const_iterator_t begin() const;
    const_iterator_t end() const;
    const_iterator_t cbegin() const;
    const_iterator_t cend() const;
};
```

The built-in node type. `name` is the node's label (any type, `std::string` by default).
`parent` is a raw, non-owning back-pointer (`nullptr` for a root). `children` owns its
subtrees via `std::unique_ptr`, so destroying a node destroys its whole subtree. `begin`/`end`
(and `const`/`c` variants) iterate over `children`, yielding `std::unique_ptr<tree_node>`.

Build a tree by constructing a root and appending children:

```cpp
using namespace cpp_utils::trees;

auto root = std::make_unique<tree_node<>>("root");
root->children.push_back(std::make_unique<tree_node<>>("node 0"));

auto& node0 = root->children[0];
node0->children.push_back(std::make_unique<tree_node<>>("node 0"));
node0->children.push_back(std::make_unique<tree_node<>>("node 1"));
```

## Making a type usable with `for_each` / `print_tree`

`for_each` and `print_tree` don't know about `tree_node` specifically — internally they only
call three free functions, also declared in `trees/node.hpp`:

```cpp
template <typename node_t> auto& child(node_t&& node, std::size_t index);
template <typename node_t> std::size_t children_count(node_t&& node);
template <typename node_t> auto name(node_t&& node);
```

These are generic templates that work for *any* type exposing:

- `.children` — indexable (`operator[]`) and sized via `std::size(...)`, holding either child
  nodes directly or (smart) pointers to them (`child()` unwraps pointer-like entries for you).
- `.name` — a plain data member (not a method) holding the node's label.

`tree_node` satisfies this shape itself, which is why it works with `for_each`/`print_tree`
out of the box. Any other aggregate with the same two members works too, with no adapter code:

```cpp
struct my_node
{
    std::string name;
    std::vector<my_node> children;
};

my_node root { "root", { my_node { "a", {} }, my_node { "b", {} } } };
for_each(root, [](auto& n) { std::cout << name(n) << "\n"; });
```

`child`, `children_count` and `name` are themselves callable directly — `for_each`'s visitor
typically calls `name(node)` to read the current node's label (as in the example above).

`trees/traits.hpp` separately provides member/method detection idioms
(`has_name_member_object_v<T>`, `has_name_method_v<T>`, `has_text_method_v<T>`, built via the
`HAS_MEMBER`/`HAS_METHOD` macros from `types/detectors.hpp`) and an internal helper,
`_get_name(const T&)`, that resolves a display name across several possible shapes — a `.name()`
method, a Qt-style `.text(0)` method, a `.toStdString()`-convertible member, or (as a fallback)
a plain member. Neither `for_each` nor `print_tree` calls `_get_name` — they use the plain
`.name`-member protocol above — so it's not something you need in order to plug a type into
these two algorithms; it exists for adapters that need to pull a name out of a foreign node
shape (e.g. Qt model/item classes) before handing it to `name`/`child`/`children_count`.

## `for_each`

```cpp
enum class traversal_order
{
    pre_order,     // visit node, then children left-to-right (default)
    post_order,    // visit children left-to-right, then node
    breadth_first  // visit level by level, root first
};

template <typename T, typename visitor_t>
void for_each(T&& tree, visitor_t&& visitor, traversal_order order = traversal_order::pre_order);
```

Visits every node of `tree` exactly once, calling `visitor(node&)` for each. `tree` can be a
node by value/reference or a (smart) pointer to one — both work, since `for_each` unwraps it
the same way `child` does. `order` selects the visit order; it defaults to `pre_order`.

Worked example, for this tree:

```
root
├── node 0
│   ├── node 0
│   ├── node 1
├── node 1
│   ├── node 0
│   ├── node 1
├── node 2
│   ├── node 0
│   ├── node 1
```

```cpp
std::vector<std::string> visited;
for_each(root, [&visited](auto& node) { visited.push_back(name(node)); });
```

- `pre_order` (default): `root, node 0, node 0, node 1, node 1, node 0, node 1, node 2, node 0, node 1`
- `post_order` (pass `traversal_order::post_order`): `node 0, node 1, node 0, node 0, node 1, node 1, node 0, node 1, node 2, root`
- `breadth_first` (pass `traversal_order::breadth_first`): `root, node 0, node 1, node 2, node 0, node 1, node 0, node 1, node 0, node 1`

## `print_tree`

```cpp
template <typename T, typename U = decltype(std::cout), int indent_increment = 3>
void print_tree(T&& tree, U& ostream = std::cout);
```

Writes an indented, box-drawing-character tree diagram of `tree` to `ostream` (default
`std::cout`). Like `for_each`, `tree` may be a node or a (smart) pointer to one. The third
template parameter, `indent_increment` (default `3`), controls how many characters each
depth level indents by.

```cpp
#include <sstream>

std::stringstream ss;
print_tree(root, ss);
```

produces:

```
root
├── node 0
│   ├── node 0
│   ├── node 1
├── node 1
│   ├── node 0
│   ├── node 1
├── node 2
│   ├── node 0
│   ├── node 1
```
