/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the “Software”), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#pragma once
#include <array>
#include <source_location>
#include <string_view>
#include <utility>

/*
 * Same family of trick as magic_enum and this library's own field_name.hpp, applied to
 * enumerators instead of aggregate members: an enum value, taken as a non-type template
 * parameter, gets rendered into the enclosing function's signature by the compiler. Unlike
 * field_name's member pointers, a plain enum value is an ordinary scalar non-type template
 * argument - portable since C++11, never subject to the P1907R1 gate that forced field_name's
 * ptr_wrapper workaround.
 *
 * What's new here is validity detection: not every integer in a scanned range names an
 * enumerator, so each candidate's rendered text is checked against a calibrated "this looks
 * like a bare identifier" shape rather than "this looks like a cast expression"
 * (e.g. GCC/Clang render an invalid value as `(ns::Enum)42`, a valid one as
 * `ns::Enum::Name`).
 */

namespace cpp_utils::reflexion::enum_name_details
{

template <typename E, E V>
consteval std::string_view raw_enum_signature()
{
    return std::string_view { std::source_location::current().function_name() };
}

enum class enum_name_calibration_probe_e
{
    probe_value = 0
};

/*
 * Same calibration idea as field_name.hpp's calibration(): render a known enumerator once,
 * then reuse the character right before its name and the text right after it to carve any
 * other enumerator's name out of its own signature, without hand-parsing each compiler's
 * exact __PRETTY_FUNCTION__ grammar. Relies on a scoped enum always rendering as
 * `Enum::Name` (the `::` right before `Name` is what calib.first locates) - true for any
 * `enum class`, namespaced or not.
 */
consteval auto calibration()
{
    constexpr std::string_view signature = raw_enum_signature<enum_name_calibration_probe_e,
        enum_name_calibration_probe_e::probe_value>();
    constexpr std::string_view needle = "probe_value";
    constexpr auto needle_pos = signature.rfind(needle);
    static_assert(needle_pos != std::string_view::npos,
        "cpp_utils::reflexion::enum_name: could not calibrate name extraction for this "
        "compiler");
    return std::pair { signature[needle_pos - 1], signature.substr(needle_pos + needle.size()) };
}

template <typename E, E V>
consteval std::string_view name_from_value()
{
    constexpr auto calib = calibration();
    constexpr std::string_view signature = raw_enum_signature<E, V>();
    constexpr auto last = signature.rfind(calib.second);
    static_assert(last != std::string_view::npos && last != 0);
    constexpr auto begin = signature.rfind(calib.first, last - 1) + 1;
    return signature.substr(begin, last - begin);
}

constexpr bool is_identifier_start(char c) noexcept
{
    return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool is_identifier_char(char c) noexcept
{
    return is_identifier_start(c) || (c >= '0' && c <= '9');
}

/*
 * An invalid value's rendering (e.g. `(ns::Enum)42` or `ns::Enum{42}`) always carries a `(`,
 * `)`, `{`, `}` or a leading digit into the extracted span, none of which pass this check -
 * a valid enumerator's own name never does. Known limitation: for an *unqualified* enum
 * (declared outside any namespace) whose invalid rendering happens to contain no `::` at
 * all, name_from_value's delimiter search falls back to the enclosing `raw_enum_signature`'s
 * own namespace-qualified name instead, which could in principle mimic an identifier. Not a
 * concern for any of this project's enums - all `enum class` types reflected here are
 * namespaced - so it's documented rather than guarded against.
 */
template <typename E, E V>
consteval bool is_valid_enumerator()
{
    constexpr std::string_view name = name_from_value<E, V>();
    if (name.empty() || !is_identifier_start(name.front()))
        return false;
    for (char c : name)
    {
        if (!is_identifier_char(c))
            return false;
    }
    return true;
}

template <typename E, int Min, int Max>
consteval auto build_enum_table()
{
    constexpr std::size_t range = static_cast<std::size_t>(Max - Min + 1);
    std::array<std::pair<E, std::string_view>, range> table {};
    std::size_t count = 0;
    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        (
            [&]
            {
                constexpr E candidate = static_cast<E>(Min + static_cast<int>(Is));
                if constexpr (is_valid_enumerator<E, candidate>())
                {
                    table[count++] = { candidate, name_from_value<E, candidate>() };
                }
            }(),
            ...);
    }(std::make_index_sequence<range> {});
    return std::pair { table, count };
}

}

namespace cpp_utils::reflexion
{

/*
 * Runtime lookup of an enumerator's own source name, e.g. enum_name(ns::Enum::Foo) == "Foo".
 * Returns an empty string_view for a value with no matching enumerator in [Min, Max]. Min/Max
 * bound the compile-time scan (a handful of extra candidates cost nothing; widen them only if
 * a real enum's values fall outside the default range).
 */
template <typename E, int Min = -8, int Max = 128>
constexpr std::string_view enum_name(E value) noexcept
{
    // Deliberately not split into a `constexpr auto& table = table_and_count.first;` local:
    // binding a reference to a subobject of a local constexpr variable defeats GCC's
    // constant-expression tracking here, turning every call into a non-constant error.
    constexpr auto table_and_count = enum_name_details::build_enum_table<E, Min, Max>();
    for (std::size_t i = 0; i < table_and_count.second; ++i)
    {
        if (table_and_count.first[i].first == value)
            return table_and_count.first[i].second;
    }
    return {};
}

}
