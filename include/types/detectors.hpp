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


// https://stackoverflow.com/questions/28309164/checking-for-existence-of-an-overloaded-member-function
#define HAS_METHOD(name, method, ...)                                                              \
    template <typename T, typename... Args>                                                        \
    struct _##name                                                                                 \
    {                                                                                              \
        template <typename C,                                                                      \
            typename = decltype(std::declval<C>().method(std::declval<Args>()...))>                \
        static std::true_type test(int);                                                           \
        template <typename C>                                                                      \
        static std::false_type test(...);                                                          \
                                                                                                   \
    public:                                                                                        \
        static constexpr bool value = decltype(test<T>(0))::value;                                 \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    struct name : std::integral_constant<bool, _##name<T, ##__VA_ARGS__>::value>                   \
    {                                                                                              \
    };                                                                                             \
    template <typename T>                                                                          \
    static inline constexpr bool name##_v = _##name<T, ##__VA_ARGS__>::value;


#define HAS_MEMBER(member)                                                                         \
    template <class T, class = void>                                                               \
    struct has_##member##_member_object : std::false_type                                          \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    struct has_##member##_member_object<T, decltype(std::declval<T>().member, void())>             \
            : std::is_member_object_pointer<decltype(&T::member)>                                  \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##member##_member_object_v                                    \
        = has_##member##_member_object<T>::value;


#define HAS_TYPE(type)                                                                             \
    template <typename T, typename = void>                                                         \
    struct has_##type##_type : std::false_type                                                     \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    struct has_##type##_type<T, decltype(std::declval<typename T::type>(), void())>                \
            : std::true_type                                                                       \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##type##_type_v = has_##type##_type<T>::value;

namespace cpp_utils::types::detectors
{


template <typename T, typename = void>
struct is_qt_tree_item : std::false_type
{
};

template <typename T>
struct is_qt_tree_item<T,
    decltype(std::declval<T>().takeChildren(), std::declval<T>().parent(),
        std::declval<T>().addChild(nullptr))> : std::true_type
{
};

template <class T>
static inline constexpr bool is_qt_tree_item_v = is_qt_tree_item<T>::value;


HAS_METHOD(has_toStdString_method, toStdString)
}
