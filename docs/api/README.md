# API Reference

One page per `cpp_utils` module, documenting every public symbol with its real signature and a
minimal, verified usage example. Start with the top-level [README](../../README.md) for the
quick-start; come here for the full picture of what each module offers.

| Module | Header dir | What it's for |
|---|---|---|
| [reflexion](reflexion.md) | `reflexion/` | Compile-time member counting & field iteration over C++20 aggregates — the mechanism `serde` is built on. |
| [serde](serde.md) | `serde/` | Symmetric binary `serialize`/`deserialize`/`runtime_size` for reflectable structs, with field wrappers for arrays, bounded strings, padding, fixed-point values, and more. |
| [endianness](endianness.md) | `endianness/` | Compile-time- and runtime-aware byte-swapping decode/encode. |
| [io](io.md) | `io/` | `buffer_view` and `memory_mapped_file` — interchangeable byte-buffer types usable directly with `serde::deserialize`. |
| [containers](containers.md) | `containers/` | `no_init_vector`, `nomap`, small container algorithms, and N-D shape/flat-indexing helpers. |
| [trees](trees.md) | `trees/` | A generic tree node plus traversal (`for_each`) and printing (`print_tree`), templatable over user-supplied node types. |
| [strings](strings.md) | `strings/` | `trim`/`ltrim`/`rtrim`, `make_unique_name`, and a subsequence `fuzzy_match` scorer. |
| [types](types.md) | `types/` | Concepts, member/method/type detection idioms, smart-pointer helpers, integer helpers, `Visitor<Ts...>`, `dispatch_dtype`. |
| [lifetime](lifetime.md) | `lifetime/` | RAII scope-leaving guards. |
| [threading](threading.md) | `threading/` | `parallel_for`/`parallel_for_each` and span-based `parallel_chunks_*` helpers. Opt-in: `-Dthreading=true`. |
| [cpp_utils_qt](cpp_utils_qt.md) | `cpp_utils_qt/` | Qt6 specializations of `types`/`trees` (smart pointers, tree items, string conversion). Opt-in: `-Dqt=true`. |
