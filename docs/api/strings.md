# `strings` module

Small, dependency-light string helpers: whitespace trimming, collision-free name
generation, a generic subsequence fuzzy-matcher, and a string-like type trait. Everything
lives in namespace `cpp_utils::strings`.

## Trim family — `trim` / `ltrim` / `rtrim`

```cpp
#include <strings/algorithms.hpp>
```

```cpp
std::string& ltrim(std::string& s);        // strip leading whitespace, in place
std::string  ltrim(const std::string& s);  // same, returns a trimmed copy

std::string& rtrim(std::string& s);        // strip trailing whitespace, in place
std::string  rtrim(const std::string& s);  // same, returns a trimmed copy

std::string& trim(std::string& s);         // strip leading and trailing whitespace, in place
std::string  trim(const std::string& s);   // same, returns a trimmed copy
```

Each function has an in-place overload (taking `std::string&`, returning a reference to the
same string) and a copying overload (taking `const std::string&`, returning a new string).
"Whitespace" is whatever `std::isspace` considers whitespace on `unsigned char`. `trim` is
just `rtrim(ltrim(s))`.

```cpp
#include <strings/algorithms.hpp>
#include <string>

using namespace cpp_utils::strings;

std::string s = "      test1     ";
trim(s);                                       // s == "test1"

std::string t = trim(std::string("   test1")); // t == "test1", original left untouched
std::string empty = trim(std::string("    ")); // empty == ""
```

Note: `algorithms.hpp` also pulls `containers::split`/`containers::join` into
`cpp_utils::strings` via `using` declarations, so they're callable as
`cpp_utils::strings::split`/`join` too — those are documented with the `containers` module,
not here.

## `make_unique_name`

```cpp
auto make_unique_name(
    const cpp_utils::types::concepts::contiguous_sequence_container auto& base_name,
    const cpp_utils::types::concepts::container auto& blacklist);
```

Returns a name derived from `base_name` that is guaranteed not to be present in
`blacklist`. If `base_name` itself isn't in `blacklist`, it's returned unchanged (a copy).
Otherwise, integer suffixes `1`, `2`, ... are appended (via `operator+` on `base_name`'s
type) until the candidate is absent from `blacklist`. The return type is
`std::decay_t<decltype(base_name)>` — the same string type as `base_name`, so this works
with `std::string`, `std::wstring`, etc. `blacklist` can be any `container` (e.g.
`std::vector<std::string>`) checked via `containers::contains`.

```cpp
#include <strings/algorithms.hpp>
#include <string>
#include <vector>

using namespace cpp_utils::strings;

// base_name not in blacklist -> returned unchanged
make_unique_name(std::string("test"), std::vector<std::string>{"test0", "test1"}); // "test"

// base_name collides -> "test1" also collides -> "test2" is free
make_unique_name(std::string("test"), std::vector<std::string>{"test", "test2"});  // "test1"
```

## `fuzzy_match`

```cpp
#include <strings/fuzzy_match.hpp>
```

```cpp
struct fuzzy_match_result
{
    int score = 0;
    std::vector<int> match_positions;
};

template <typename CharT>
int fuzzy_match_score(
    std::basic_string_view<CharT> query, std::basic_string_view<CharT> candidate);

template <typename CharT>
fuzzy_match_result fuzzy_match(
    std::basic_string_view<CharT> query, std::basic_string_view<CharT> candidate);
```

Both functions score how well `query` matches as an ordered (non-contiguous) subsequence
of `candidate` — the classic fuzzy-finder model used by editor "go to file"/"command
palette" pickers, e.g. `"wl"` matching `"WriteLog"` inside a longer identifier.
`fuzzy_match_score` returns just the integer score; `fuzzy_match` additionally reports,
in `match_positions`, one `candidate` index per `query` character (in increasing order)
for the best-scoring alignment found.

Scoring model:
- The matcher considers *every* valid subsequence alignment (via dynamic programming, not
  a greedy left-to-right scan) and keeps the highest-scoring one, so a clean
  word-boundary-aligned match later in the string beats a "throwaway" greedy match on an
  earlier stray character.
- Matched characters at the very start of `candidate`, right after a separator (`_`, ` `,
  `/`), or at a lowercase→Uppercase camelCase transition score a bonus over a mid-word
  match.
- Consecutive matched characters (no gap between them) score a further adjacency bonus.
- The raw score is then normalized by how much of `candidate` was *not* matched — more
  leftover, unmatched candidate length lowers the score — and floored at `1` for any
  successful match.
- Return value: `0` means `query` is not a subsequence of `candidate` at all (or
  `candidate` is empty while `query` isn't); an empty `query` always scores `1` against
  any `candidate`. Otherwise the score is a positive `int` with no fixed upper bound —
  it's only meaningful to *compare* scores against each other (e.g. to rank candidates),
  not to read as a percentage or fixed scale.

Matching is case-insensitive, but only within the ASCII range: `CharT` must behave like
`char` or `wchar_t` under `std::tolower`/`std::isupper`/`std::islower` (or their wide
`std::towlower`/`std::iswupper`/`std::iswlower` equivalents). This is **not** a full
Unicode case-fold — non-ASCII characters are compared as-is.

```cpp
#include <strings/fuzzy_match.hpp>

using namespace cpp_utils::strings;

// Word-boundary/camelCase-aligned match: "wl" picks out the 'W' and 'L' that start
// words inside "someWriteLogFunction", scoring higher than matching any incidental
// lowercase 'w'/'l' earlier in the string.
int score = fuzzy_match_score<char>("wl", "someWriteLogFunction");
// score > 0

// No subsequence match at all -> 0.
fuzzy_match_score<char>("xyz", "abcdef"); // == 0

// fuzzy_match() also reports the matched positions, one per query character.
auto result = fuzzy_match<char>("abc", "aXbXc");
// result.score > 0
// result.match_positions == {0, 2, 4}, strictly increasing
```

## Traits

```cpp
#include <strings/traits.hpp>
```

```cpp
template <typename T>
struct is_string_like
        : std::disjunction<containers::is_std_basic_string<std::decay_t<T>>,
              std::is_same<std::decay_t<T>, std::string_view>,
              std::is_same<std::decay_t<T>, const char*>, std::is_same<std::decay_t<T>, char*>>
{
};

template <typename T>
static inline constexpr bool is_string_like_v = is_string_like<T>::value;
```

`is_string_like<T>` (and the `_v` convenience variable) detects whether `T`, after
stripping cv/reference qualifiers, is a "string-like" type: any `std::basic_string<...>`
instantiation (e.g. `std::string`, `std::wstring`), `std::string_view`, or a raw
`char*`/`const char*`. Useful for constraining templates that need to special-case string
arguments (e.g. to avoid treating them as generic containers).

```cpp
#include <strings/traits.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace cpp_utils::strings;

static_assert(is_string_like_v<std::string>);
static_assert(is_string_like_v<std::string_view>);
static_assert(is_string_like_v<const char*>);
static_assert(!is_string_like_v<int>);
static_assert(!is_string_like_v<std::vector<int>>);
```
