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

#include <memory>
#include <type_traits>

namespace cpp_utils::types::pointers
{

template <typename T, typename = void>
struct is_dereferencable : std::false_type
{
};

template <typename T>
struct is_dereferencable<T, decltype(*std::declval<T>(), void())> : std::true_type
{
};

template <class T>
static inline constexpr bool is_dereferencable_v = is_dereferencable<T>::value;

template <typename T, typename = void>
struct is_std_shared_ptr : std::false_type
{
};

template <typename T>
struct is_std_shared_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::shared_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_shared_ptr_v = is_std_shared_ptr<T>::value;

template <typename T, typename = void>
struct is_std_unique_ptr : std::false_type
{
};

template <typename T>
struct is_std_unique_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::unique_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_unique_ptr_v = is_std_unique_ptr<T>::value;

template <typename T, typename = void>
struct is_std_weak_ptr : std::false_type
{
};

template <typename T>
struct is_std_weak_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::weak_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_weak_ptr_v = is_std_weak_ptr<T>::value;


template <typename T>
struct is_std_smart_ptr
        : std::disjunction<is_std_shared_ptr<T>, is_std_unique_ptr<T>, is_std_weak_ptr<T>>
{
};

template <class T>
static inline constexpr bool is_std_smart_ptr_v = is_std_smart_ptr<T>::value;

template <typename T, typename = void>
struct is_smart_ptr : std::false_type
{
};

template <typename T>
struct is_smart_ptr<T, std::enable_if_t<std::disjunction_v<is_std_smart_ptr<T>>>> : std::true_type
{
};

template <class T>
static inline constexpr bool is_smart_ptr_v = is_smart_ptr<T>::value;

template <typename T>
constexpr auto to_value(T&& item)
{
    if constexpr (std::is_pointer_v<std::remove_reference_t<std::remove_cv_t<T>>>)
    {
        return *item;
    }
    else
    {
        if constexpr (is_smart_ptr_v<std::remove_reference_t<std::remove_cv_t<T>>>)
        {
            return *item.get();
        }
        else
        {
            return item;
        }
    }
}

template <typename T>
constexpr auto& to_value_ref(T&& item)
{
    if constexpr (std::is_pointer_v<std::remove_reference_t<std::remove_cv_t<T>>>)
    {
        return *item;
    }
    else
    {
        if constexpr (is_smart_ptr_v<std::remove_reference_t<std::remove_cv_t<T>>>)
        {
            return *item.get();
        }
        else
        {
            return item;
        }
    }
}
}
