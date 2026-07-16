#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2019, Plasma Physics Laboratory - CNRS
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
--                            alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#include <concepts>
#include <type_traits>

//TODO find a way to merge IS_TEMPLATE_T with IS_T
#define IS_TEMPLATE_T(name, type)                                                                  \
    template <typename T>                                                                          \
    struct name : std::false_type                                                                  \
    {                                                                                              \
    };                                                                                             \
    template <typename ...T>                                                                          \
    struct name<type<T...>> : std::true_type                                                          \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    static inline constexpr bool name##_v = name<T>::value;

#define IS_T(name, type)                                                                           \
    template <typename T>                                                                          \
    struct name : std::false_type                                                                  \
    {                                                                                              \
    };                                                                                             \
    template <>                                                                                    \
    struct name<type> : std::true_type                                                             \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    static inline constexpr bool name##_v = name<T>::value;


// A struct wrapper (not a bare concept) is required so these traits stay usable as types
// (e.g. std::conjunction<has_begin_method<T>, has_end_method<T>, ...> in containers/traits.hpp).
#define HAS_METHOD(name, method, ...)                                                              \
    template <typename T, typename... Args>                                                        \
    concept _##name##_concept                                                                      \
        = requires(T t) { t.method(std::declval<Args>()...); };                                    \
                                                                                                   \
    template <typename T>                                                                          \
    struct name : std::bool_constant<_##name##_concept<T, ##__VA_ARGS__>>                          \
    {                                                                                              \
    };                                                                                             \
    template <typename T>                                                                          \
    static inline constexpr bool name##_v = _##name##_concept<T, ##__VA_ARGS__>;


#define HAS_MEMBER(member)                                                                         \
    template <class T>                                                                             \
    concept _has_##member##_member_object_concept                                                  \
        = requires(T t) { requires std::is_member_object_pointer_v<decltype(&T::member)>; };       \
                                                                                                   \
    template <class T>                                                                             \
    struct has_##member##_member_object                                                            \
        : std::bool_constant<_has_##member##_member_object_concept<T>>                             \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##member##_member_object_v                                    \
        = _has_##member##_member_object_concept<T>;


#define HAS_TYPE(type)                                                                             \
    template <typename T>                                                                          \
    concept _has_##type##_type_concept = requires { typename T::type; };                           \
                                                                                                   \
    template <typename T>                                                                          \
    struct has_##type##_type : std::bool_constant<_has_##type##_type_concept<T>>                   \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##type##_type_v = _has_##type##_type_concept<T>;

namespace cpp_utils::types::detectors
{


template <typename T>
concept _is_qt_tree_item_concept = requires(T t) {
    t.takeChildren();
    t.parent();
    t.addChild(nullptr);
};

template <typename T>
struct is_qt_tree_item : std::bool_constant<_is_qt_tree_item_concept<T>>
{
};

template <class T>
static inline constexpr bool is_qt_tree_item_v = _is_qt_tree_item_concept<T>;

template <typename ref_type, typename... types>
struct is_any_of : std::integral_constant<bool,(std::is_same_v<ref_type, types> || ...)>{};

template <typename ref_type, typename... types>
using is_any_of_t = typename is_any_of<ref_type,types...>::type;

template <typename ref_type, typename... types>
inline constexpr bool is_any_of_v = is_any_of<ref_type,types...>::value;


HAS_METHOD(has_toStdString_method, toStdString)
}
