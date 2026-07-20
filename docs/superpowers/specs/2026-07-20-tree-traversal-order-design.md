# Design: `traversal_order` for `trees::for_each`

## Context

`cpp_utils::trees::for_each` (`include/cpp_utils/trees/algorithms.hpp`) currently has exactly
one traversal order hardcoded: pre-order depth-first (visit the node, then recurse into its
children left-to-right). There is no way to visit children before their parent (post-order,
useful for bottom-up aggregation) or level by level (breadth-first).

This was raised while auditing test coverage on `trees/algorithms.hpp` and discussed as an
exploratory question first. The initial recommendation was YAGNI — wait for a concrete
consumer — but the decision was to add it now: as a general-purpose library, the traversal
order is a natural, low-cost thing to support proactively rather than re-deriving and adding
later under time pressure.

## API

Add an enum and a runtime default-argument parameter to the existing `for_each`, declared in
`trees/algorithms.hpp` next to it (no new header — mirrors how `containers::array_order` lives
directly in `nd_shape.hpp` beside the one function, `flat_index`, that uses it):

```cpp
enum class traversal_order
{
    pre_order,     // visit node, then children left-to-right (current/default behavior)
    post_order,    // visit children left-to-right, then the node
    breadth_first  // visit level by level, root first
};

template <typename T, typename visitor_t>
void for_each(T&& tree, visitor_t&& visitor, traversal_order order = traversal_order::pre_order);
```

Existing call sites (`for_each(tree, visitor)`) keep compiling unchanged; `pre_order` stays the
default, matching current behavior exactly.

### Why a runtime default-arg enum, not a template parameter

The initial draft of this design proposed a non-type template parameter
(`for_each<traversal_order::post_order>(tree, visitor)`) for zero-cost dispatch, following the
codebase's compile-time tag-dispatch idiom used elsewhere (`Visitor<Ts...>`, endianness tags,
`SPLIT_FIELDS` `if constexpr` cascades). That idiom is a strong fit for *type-level* metadata
(how a struct's fields are laid out on the wire), but `containers::array_order` is a closer
functional analog — an algorithm-variant selector with a sensible default, exactly this
scenario — and it's a runtime default-argument enum consumed by `flat_index`. Following that
precedent keeps the two closest-analog APIs in the library consistent. It also costs less than
it first appears: `breadth_first` needs a heap-allocated queue regardless of dispatch
mechanism, so the "zero-cost" argument for a template parameter only really applies to
`pre_order`/`post_order`, and a single branch on a 3-way enum at the top of `for_each` is
negligible next to that.

## Implementation

Three small `details::` helpers, selected via a `switch` in the public `for_each`:

- `_for_each` (existing, unchanged): `visitor(node)`, then recurse into each child.
- `_for_each_post_order` (new): recurse into each child, then `visitor(node)`.
- `_for_each_breadth_first` (new): `std::queue<std::reference_wrapper<node_t>>` seeded with the
  root; loop: pop front, visit, push all of its children; repeat until empty. `node_t` is
  deduced the same way the rest of the file already handles node references
  (`std::remove_reference_t<decltype(to_value_ref(tree))>`), so it works for any node type
  satisfying the existing tree traits, not just `tree_node`.

No change to the visitor signature — all three orders call `visitor(node)`, only the sequence
changes.

## Testing

Added to `tests/trees/test_print.cpp`, reusing the existing `make_test_tree()` fixture already
used by the pre-order test (a root with 3 children, each of which has 2 children of its own —
10 nodes total):

- **Post-order** visits children before parent — exact expected sequence pinned down
  (`node 0, node 1, node 0, node 0, node 1, node 1, node 0, node 1, node 2, root`).
- **Breadth-first** visits level by level, root first — exact expected sequence pinned down
  (`root, node 0, node 1, node 2, node 0, node 1, node 0, node 1, node 0, node 1`).
- **Single node, no children** behaves identically under all three orders (the trivial base
  case each traversal must degrade to correctly — cheap to pin down and exactly the class of
  corner case the rest of this session's coverage audit was about).

## Documentation

Update the `trees/` bullet in `CLAUDE.md` to mention the new `traversal_order` parameter and
its three values.

## Out of scope

`print_tree`'s internal traversal (`details::_print_tree`) stays untouched. It needs
indent-level tracking that `for_each`'s plain `visitor(node)` signature doesn't carry, so it
isn't a natural consumer of `traversal_order`, and refactoring it to share code with `for_each`
is an unrelated change this task doesn't need.
