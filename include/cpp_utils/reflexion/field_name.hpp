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
 * Declared, never defined: only ever used inside consteval functions to form the address of
 * one of its subobjects, which never requires the object to actually be constructed or
 * linked. This gives every member of T a stable, NTTP-usable pointer without requiring T to
 * be default-constructible. Clang warns that the never-provided definition looks like an ODR
 * hazard; it isn't one here since nothing ever reads through the pointer, only its address
 * feeds into a consteval-only signature lookup.
 */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif
template <typename T>
struct static_probe
{
    static T instance;
};

template <typename T, std::size_t N>
consteval auto field_ptr()
{
    return std::get<N>(cpp_utils_reflexion_field_ptr_tuple(static_probe<T>::instance));
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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
