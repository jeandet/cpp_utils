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
#include <type_traits>
#include <utility>
#include "../types/concepts.hpp"

/*
 * see https://stackoverflow.com/questions/39768517/structured-bindings-width/39779537#39779537
 */

namespace details
{
struct anything
{
    template <class T> requires (!std::is_bounded_array_v<std::decay_t<T>>)
    operator T() const;
};

template<typename T, typename... A0>
consteval auto MemberCounter(auto ...c0) {
    if constexpr (requires { T{ {A0{}}..., {anything{}}, c0... }; } == false
               && requires { T{ {A0{}}..., c0..., anything{}}; } == false )
        return sizeof...(A0) + sizeof...(c0);
    else
    if constexpr (requires { T{ {A0{}}..., {anything{}}, c0... }; })
        return MemberCounter<T,A0...,anything>(c0...);
    else
        return MemberCounter<T,A0...>(c0...,anything{});
}

}


#define SPLIT_FIELDS_FW_DECL(return_type, name, const_struct)                                      \
    template <typename T, typename... Args>                                                        \
    return_type name(const_struct T& structure, Args&&... args);


#define SPLIT_FIELDS(return_type, name, function, const_struct)                                    \
    /* this looks quite ugly bit it is worth it!*/                                                 \
    template <typename T, typename... Args>                                                        \
    return_type name(const_struct T& structure, Args&&... args)                                    \
    {                                                                                              \
        constexpr std::size_t count = cpp_utils::reflexion::count_members<T>;                                            \
        static_assert(count <= 31);                                                                \
                                                                                                   \
        if constexpr (count == 1)                                                                  \
        {                                                                                          \
            auto& [_0] = structure;                                                                \
            return function(structure, args..., _0);                                               \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 2)                                                                  \
        {                                                                                          \
            auto& [_0, _1] = structure;                                                            \
            return function(structure, args..., _0, _1);                                           \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 3)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2] = structure;                                                        \
            return function(structure, args..., _0, _1, _2);                                       \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 4)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3] = structure;                                                    \
            return function(structure, args..., _0, _1, _2, _3);                                   \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 5)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4] = structure;                                                \
            return function(structure, args..., _0, _1, _2, _3, _4);                               \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 6)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5] = structure;                                            \
            return function(structure, args..., _0, _1, _2, _3, _4, _5);                           \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 7)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6] = structure;                                        \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6);                       \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 8)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7] = structure;                                    \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7);                   \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 9)                                                                  \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8] = structure;                                \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8);               \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 10)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9] = structure;                            \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9);           \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 11)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10] = structure;                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10);      \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 12)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11] = structure;                  \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11); \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 13)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12] = structure;             \
            return function(                                                                       \
                structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);        \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 14)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13] = structure;        \
            return function(                                                                       \
                structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13);   \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 15)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14] = structure;   \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14);                                                                    \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 16)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15]           \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15);                                                               \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 17)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16]      \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16);                                                          \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 18)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17] \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17);                                                     \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 19)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18]                                                                               \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18);                                                \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 20)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19]                                                                          \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19);                                           \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 21)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20]                                                                     \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20);                                      \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 22)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21]                                                                \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21);                                 \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 23)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22]                                                           \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22);                            \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 24)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23]                                                      \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23);                       \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 25)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24]                                                 \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24);                  \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 26)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25]                                            \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25);             \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 27)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25, _26]                                       \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26);        \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 28)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25, _26, _27]                                  \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27);   \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 29)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28]                             \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27,    \
                _28);                                                                              \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 30)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29]                        \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27,    \
                _28, _29);                                                                         \
        }                                                                                          \
                                                                                                   \
        if constexpr (count == 31)                                                                 \
        {                                                                                          \
            auto& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, \
                _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30]                   \
                = structure;                                                                       \
            return function(structure, args..., _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,  \
                _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27,    \
                _28, _29, _30);                                                                    \
        }                                                                                          \
    }


namespace cpp_utils::reflexion
{

struct field_tag_t
{
};

template <typename composite_t>
consteval auto is_field()
{
    if constexpr (requires { typename composite_t::field_tag; })
    {
        return true;
    }
    else
    {
        return false;
    }
}

template <typename composite_t>
inline constexpr bool is_field_v = is_field<composite_t>();

template <typename T>
inline constexpr std::size_t count_members = details::MemberCounter<T>();

SPLIT_FIELDS_FW_DECL([[nodiscard]] constexpr std::size_t, composite_size, const);
template <typename composite_t>
consteval std::size_t composite_size();

template <typename field_t>
[[nodiscard]] consteval std::size_t field_size()
{
    if constexpr (std::is_bounded_array_v<field_t>)
        return sizeof(field_t)*std::extent_v<field_t>;
    return sizeof(field_t);
}

[[nodiscard]] constexpr inline std::size_t fields_size(const auto& , auto& field)
{
    using Field_t = std::decay_t<decltype(field)>;
    if constexpr (std::is_compound_v<Field_t> && ! std::is_bounded_array_v<Field_t> && !is_field_v<Field_t>)
        return composite_size<Field_t>();
    else
        return field_size<Field_t>();
}

template <typename record_t, typename T, typename... Ts>
[[nodiscard]] constexpr inline std::size_t fields_size(const record_t& s, T&& field, Ts&&... fields)
{
    return fields_size(s, std::forward<T>(field)) + fields_size(s, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS([[nodiscard]] constexpr std::size_t, composite_size, fields_size, const);

template <typename composite_t>
consteval std::size_t composite_size()
{
    return composite_size(composite_t{});
}

} // namespace cpp_utils::reflexion
