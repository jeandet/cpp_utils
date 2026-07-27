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
#include "reflection.hpp"
#include <source_location>
#include <string_view>
#include <tuple>
#include <utility>

/*
 * Same family of trick as magic_enum, applied to aggregate members instead of enumerators:
 * a member's address, taken as a non-type template parameter, gets rendered into the
 * enclosing function's signature by the compiler. We recover the name by locating it in
 * that signature.
 *
 * see https://github.com/yosh-matsuda/field-reflection (same technique, independent
 * implementation reusing this file's own SPLIT_FIELDS-based field enumeration).
 */

namespace cpp_utils::reflexion::details
{

constexpr auto make_field_ptr_tuple(const auto&, auto&... fields)
{
    return std::make_tuple(&fields...);
}

}

SPLIT_FIELDS(constexpr auto, cpp_utils_reflexion_field_ptr_tuple, cpp_utils::reflexion::details::make_field_ptr_tuple, const);

namespace cpp_utils::reflexion::details
{

/*
 * A real, defined static instance - only ever used inside consteval functions to form the
 * address of one of its subobjects, never to read through it. This mechanism is only ever
 * reached for T that count_members<T> already accepts, which in practice means T is
 * default-constructible (an aggregate with a reference member, the one common way to defeat
 * default-construction, already fails count_members's own probe before getting here - see
 * reflection.hpp). An earlier version of this left `instance` declared-but-never-defined to
 * further avoid requiring default-constructibility; that turned out to be more than
 * count_members's own reach ever needed, so it was simplified to a genuine definition -
 * uncontroversial, portable C++.
 */
template <typename T>
inline T static_probe_instance {};

/*
 * A bare pointer-to-subobject used directly as a non-type template argument needs P1907R1
 * ("generalized non-type template arguments" - WG21, still an experimental extension, not a
 * ratified feature: __cpp_nontype_template_args isn't bumped for it). GCC has accepted this
 * since GCC 11; Clang only since Clang 18 (2024) - confirmed failing on this project's
 * macOS wheel-building CI, which ships Clang 15. Wrapping the pointer in a one-member
 * aggregate and letting CTAD deduce *that* as the template argument sidesteps the gap
 * entirely: a class-type template argument built from a pointer member is covered by
 * P0732 ("class types as non-type template parameters"), a much older, widely-supported
 * C++20 feature - Clang has accepted it for years. Same workaround Boost.PFR uses for its
 * own C++20 field-name reflection (boost::pfr::detail::clang_wrapper_t), for the identical
 * reason - see that project's core_name20_static.hpp.
 */
template <typename T>
struct ptr_wrapper
{
    T value;
};
template <typename T>
ptr_wrapper(T) -> ptr_wrapper<T>;

template <typename T, std::size_t N>
consteval auto field_ptr()
{
    return ptr_wrapper { std::get<N>(cpp_utils_reflexion_field_ptr_tuple(static_probe_instance<T>)) };
}

template <typename T, auto Ptr>
consteval std::string_view raw_signature()
{
    return std::string_view { std::source_location::current().function_name() };
}

struct calibration_probe_t
{
    int probe_field;
};

/*
 * Every compiler renders a member-pointer NTTP differently in a function signature
 * (`&s.field`, `.field`, `->field`, ...), but for a *given* compiler the character right
 * before the name and the text right after it are stable across signatures. Calibrate both
 * once against a field whose name we already know, then reuse that shape to carve the name
 * out of any other signature - avoids hand-parsing each compiler's exact
 * __PRETTY_FUNCTION__/__FUNCSIG__ grammar.
 */
consteval auto calibration()
{
    constexpr std::string_view signature
        = raw_signature<calibration_probe_t, field_ptr<calibration_probe_t, 0>()>();
    constexpr std::string_view needle = "probe_field";
    constexpr auto needle_pos = signature.rfind(needle);
    static_assert(needle_pos != std::string_view::npos,
        "cpp_utils::reflexion::field_name: could not calibrate name extraction for this "
        "compiler");
    return std::pair { signature[needle_pos - 1], signature.substr(needle_pos + needle.size()) };
}

template <typename T, auto Ptr>
consteval std::string_view name_from_ptr()
{
    constexpr auto calib = calibration();
    constexpr std::string_view signature = raw_signature<T, Ptr>();
    constexpr auto last = signature.rfind(calib.second);
    static_assert(last != std::string_view::npos && last != 0);
    constexpr auto begin = signature.rfind(calib.first, last - 1) + 1;
    return signature.substr(begin, last - begin);
}

}

namespace cpp_utils::reflexion
{

template <typename T, std::size_t N>
inline constexpr std::string_view field_name
    = details::name_from_ptr<T, details::field_ptr<T, N>()>();

}
