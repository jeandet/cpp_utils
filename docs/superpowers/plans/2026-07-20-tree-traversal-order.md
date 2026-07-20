# Tree Traversal Order Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `traversal_order` parameter (`pre_order` default / `post_order` / `breadth_first`) to `cpp_utils::trees::for_each`, per the approved design in `docs/superpowers/specs/2026-07-20-tree-traversal-order-design.md`.

**Architecture:** One enum plus two new `details::` helper functions in `include/cpp_utils/trees/algorithms.hpp`, dispatched from the existing `for_each` via a `switch` on a new default-valued third parameter. No new files, no signature changes to the visitor callback, no changes to `print_tree`.

**Tech Stack:** C++20 (header-only), Catch2 v3 (`TEST_CASE`/`REQUIRE`, plain style — this test file does not use `GIVEN`/`WHEN`/`THEN`), Meson/Ninja.

## Global Constraints

- `for_each(tree, visitor)` (2-argument form) must keep compiling unchanged — `pre_order` is the default.
- `traversal_order` is declared directly in `trees/algorithms.hpp` next to `for_each` — no new header (mirrors `containers::array_order` living in `nd_shape.hpp` beside `flat_index`).
- No change to `print_tree` / `details::_print_tree` — explicitly out of scope per the design spec.
- Every new code path gets a test asserting the exact expected visit sequence, not just "it ran."
- C++20 only (`cpp_std=c++20` is hard-pinned project-wide); formatting follows `.clang-format` (WebKit-based, Allman braces, 4-space indent, 100 col).
- Build/test via Meson: `meson setup build` (if not already configured), `ninja -C build`, `meson test -C build <Test-Name>`.

---

## Task 1: `post_order` traversal

**Files:**
- Modify: `include/cpp_utils/trees/algorithms.hpp` (details namespace ~line 36-44; public API ~line 68-73)
- Test: `tests/trees/test_print.cpp` (append after the existing "Depth-first traversal..." `TEST_CASE`, ~line 62)

**Interfaces:**
- Consumes: existing `details::_for_each`, `children_count(node)`, `child(node, i)` from `trees/node.hpp` (unchanged).
- Produces: `enum class cpp_utils::trees::traversal_order { pre_order, post_order };` and `template <typename T, typename visitor_t> void cpp_utils::trees::for_each(T&& tree, visitor_t&& visitor, traversal_order order = traversal_order::pre_order);` — both extended again in Task 2, but must compile and pass standalone after this task.

- [ ] **Step 1: Write the failing tests**

Append to `tests/trees/test_print.cpp`, right after the existing `TEST_CASE("Depth-first traversal visits every node exactly once, root first", "[trees]")` block:

```cpp
TEST_CASE("Post-order traversal visits children before the node, root last", "[trees]")
{
    std::vector<std::string> visited;
    for_each(
        make_test_tree(), [&visited](auto& node) { visited.push_back(name(node)); },
        traversal_order::post_order);
    REQUIRE(visited
        == std::vector<std::string> { "node 0", "node 1", "node 0", "node 0", "node 1", "node 1",
            "node 0", "node 1", "node 2", "root" });
}

TEST_CASE("for_each with an explicit pre_order argument matches the default", "[trees]")
{
    std::vector<std::string> visited;
    for_each(
        make_test_tree(), [&visited](auto& node) { visited.push_back(name(node)); },
        traversal_order::pre_order);
    REQUIRE(visited
        == std::vector<std::string> { "root", "node 0", "node 0", "node 1", "node 1", "node 0",
            "node 1", "node 2", "node 0", "node 1" });
}
```

The post-order expected sequence comes from `make_test_tree()`'s fixed shape (root with children `node 0`, `node 1`, `node 2`, each of which has grandchildren `node 0`, `node 1`): for each of root's 3 children, its 2 grandchildren are visited first, then the child itself; root is visited last.

- [ ] **Step 2: Run tests to verify they fail**

```bash
meson setup build   # only if build/ doesn't already exist
ninja -C build tests/TreePrint
```

Expected: compile failure — `traversal_order` is not declared and `for_each` does not accept a third argument.

- [ ] **Step 3: Write the minimal implementation**

In `include/cpp_utils/trees/algorithms.hpp`, add a new helper inside the existing `namespace details { ... }` block, directly after `_for_each` (i.e. after its closing `}` at what is currently line 44, before the blank line preceding `_print_tree`):

```cpp
    template <typename T, typename visitor_t>
    void _for_each_post_order(T&& node, visitor_t& visitor)
    {
        for (std::size_t i = 0; i < children_count(node); i++)
            _for_each_post_order(child(node, i), visitor);
        visitor(node);
    }
```

Then replace the current public `for_each` (currently lines 68-73):

```cpp
template <typename T, typename visitor_t>
void for_each(T&& tree, visitor_t&& visitor)
{
    using namespace cpp_utils::types::pointers;
    details::_for_each(to_value_ref(tree), visitor);
}
```

with:

```cpp
enum class traversal_order
{
    pre_order, // visit node, then children left-to-right (default)
    post_order // visit children left-to-right, then node
};

template <typename T, typename visitor_t>
void for_each(T&& tree, visitor_t&& visitor, traversal_order order = traversal_order::pre_order)
{
    using namespace cpp_utils::types::pointers;
    switch (order)
    {
        case traversal_order::pre_order:
            details::_for_each(to_value_ref(tree), visitor);
            break;
        case traversal_order::post_order:
            details::_for_each_post_order(to_value_ref(tree), visitor);
            break;
    }
}
```

- [ ] **Step 4: Run tests to verify they pass**

```bash
ninja -C build tests/TreePrint
meson test -C build Test-TreePrint
```

Expected: `Ok: 1` (all `TEST_CASE`s in this binary pass, including the 2 new ones and the pre-existing pre-order/print tests).

- [ ] **Step 5: Commit**

```bash
git add include/cpp_utils/trees/algorithms.hpp tests/trees/test_print.cpp
git commit -m "feat: add trees::traversal_order with pre_order/post_order support"
```

---

## Task 2: `breadth_first` traversal

**Files:**
- Modify: `include/cpp_utils/trees/algorithms.hpp` (includes; details namespace; public API)
- Test: `tests/trees/test_print.cpp` (append after Task 1's new tests)

**Interfaces:**
- Consumes: Task 1's `traversal_order` enum and `for_each` (extends both in place), `children_count`/`child` from `trees/node.hpp`.
- Produces: final 3-value `traversal_order` (`pre_order`, `post_order`, `breadth_first`) and the final `for_each` covering all three — this is the complete public API, unchanged by later tasks.

- [ ] **Step 1: Write the failing test**

Append to `tests/trees/test_print.cpp`:

```cpp
TEST_CASE("Breadth-first traversal visits level by level, root first", "[trees]")
{
    std::vector<std::string> visited;
    for_each(
        make_test_tree(), [&visited](auto& node) { visited.push_back(name(node)); },
        traversal_order::breadth_first);
    REQUIRE(visited
        == std::vector<std::string> { "root", "node 0", "node 1", "node 2", "node 0", "node 1",
            "node 0", "node 1", "node 0", "node 1" });
}
```

Expected sequence: depth 0 is `root`; depth 1 is root's 3 children in order (`node 0`, `node 1`, `node 2`); depth 2 is each depth-1 node's 2 children, processed in the same left-to-right order as their parents were visited.

- [ ] **Step 2: Run test to verify it fails**

```bash
ninja -C build tests/TreePrint
```

Expected: compile failure — `traversal_order::breadth_first` is not a declared enumerator.

- [ ] **Step 3: Write the minimal implementation**

In `include/cpp_utils/trees/algorithms.hpp`, add two new system includes alongside the existing `#include <iostream>`:

```cpp
#include <functional>
#include <iostream>
#include <queue>
#include <type_traits>
```

Add a new helper inside `namespace details { ... }`, directly after `_for_each_post_order`:

```cpp
    template <typename T, typename visitor_t>
    void _for_each_breadth_first(T&& node, visitor_t& visitor)
    {
        using node_t = std::remove_reference_t<T>;
        std::queue<std::reference_wrapper<node_t>> pending;
        pending.push(node);
        while (!pending.empty())
        {
            node_t& current = pending.front();
            pending.pop();
            visitor(current);
            for (std::size_t i = 0; i < children_count(current); i++)
                pending.push(child(current, i));
        }
    }
```

Replace the `traversal_order` enum and `for_each` from Task 1 with:

```cpp
enum class traversal_order
{
    pre_order,    // visit node, then children left-to-right (default)
    post_order,   // visit children left-to-right, then node
    breadth_first // visit level by level, root first
};

template <typename T, typename visitor_t>
void for_each(T&& tree, visitor_t&& visitor, traversal_order order = traversal_order::pre_order)
{
    using namespace cpp_utils::types::pointers;
    switch (order)
    {
        case traversal_order::pre_order:
            details::_for_each(to_value_ref(tree), visitor);
            break;
        case traversal_order::post_order:
            details::_for_each_post_order(to_value_ref(tree), visitor);
            break;
        case traversal_order::breadth_first:
            details::_for_each_breadth_first(to_value_ref(tree), visitor);
            break;
    }
}
```

- [ ] **Step 4: Run test to verify it passes**

```bash
ninja -C build tests/TreePrint
meson test -C build Test-TreePrint
```

Expected: `Ok: 1`, all `TEST_CASE`s pass (including Task 1's).

- [ ] **Step 5: Commit**

```bash
git add include/cpp_utils/trees/algorithms.hpp tests/trees/test_print.cpp
git commit -m "feat: add trees::traversal_order::breadth_first"
```

---

## Task 3: single-node corner case across all three orders

**Files:**
- Test: `tests/trees/test_print.cpp` (append after Task 2's new test)

**Interfaces:**
- Consumes: the final `traversal_order` enum and `for_each` from Task 2. No production code changes in this task.

- [ ] **Step 1: Write the test**

```cpp
TEST_CASE("A single node with no children visits just itself under every traversal order",
    "[trees]")
{
    for (auto order : { traversal_order::pre_order, traversal_order::post_order,
             traversal_order::breadth_first })
    {
        std::vector<std::string> visited;
        for_each(
            std::make_unique<tree_node<>>("solo"),
            [&visited](auto& node) { visited.push_back(name(node)); }, order);
        REQUIRE(visited == std::vector<std::string> { "solo" });
    }
}
```

- [ ] **Step 2: Run test to verify it passes**

```bash
ninja -C build tests/TreePrint
meson test -C build Test-TreePrint
```

Expected: `Ok: 1`. This task adds no new production code, so the test should pass immediately — if it fails, that means Task 1 or 2's implementation has a base-case bug (e.g. `_for_each_breadth_first` mishandling a childless root), which must be fixed in `include/cpp_utils/trees/algorithms.hpp` before proceeding.

- [ ] **Step 3: Commit**

```bash
git add tests/trees/test_print.cpp
git commit -m "test: pin down single-node behavior across all trees::traversal_order values"
```

---

## Task 4: Documentation

**Files:**
- Modify: `CLAUDE.md:81-82`

**Interfaces:**
- Consumes: the final public API from Task 2 (documents it; no code).

- [ ] **Step 1: Update the `trees/` bullet**

Current text (`CLAUDE.md:81-82`):

```markdown
- `trees/` — a generic tree `node<T>` plus traversal algorithms and trait detection so trees can
  be templated over user-supplied node-like types.
```

Replace with:

```markdown
- `trees/` — a generic tree `node<T>` plus traversal algorithms and trait detection so trees can
  be templated over user-supplied node-like types. `for_each` takes an optional `traversal_order`
  (`pre_order` default, `post_order`, `breadth_first`) selecting visit order at runtime, mirroring
  `containers::array_order`'s default-argument-enum pattern.
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: document trees::traversal_order in CLAUDE.md"
```

---

## Final Verification

- [ ] **Step 1: Run the full test suite**

```bash
meson test -C build
```

Expected: every test passes (`Ok: <N>`, `Fail: 0`), matching the count from before this plan plus no regressions — confirm by reading the actual `Ok`/`Fail` counts and exit code, not a partial grep.

- [ ] **Step 2: Update project memory and confirm push preference**

Per this repo's CLAUDE.md workflow rules: update the relevant project memory with what shipped, then commit is already done per-task above. Do not push without explicit user consent for this round, even though a prior round in this session was pushed — confirm again.
