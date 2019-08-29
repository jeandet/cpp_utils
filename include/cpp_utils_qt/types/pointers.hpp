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
#include "../../types/pointers.hpp"
#include <QExplicitlySharedDataPointer>
#include <QScopedArrayPointer>
#include <QScopedPointer>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QWeakPointer>
#include <type_traits>

namespace cpp_utils::types::pointers
{

template <typename T, typename = void>
struct is_QExplicitlySharedDataPointer : std::false_type
{
};

template <typename T>
struct is_QExplicitlySharedDataPointer<T, decltype(std::declval<typename T::Type>(), void())>
        : std::is_same<T, QExplicitlySharedDataPointer<typename T::Type>>
{
};

template <class T>
static inline constexpr bool is_QExplicitlySharedDataPointer_v
    = is_QExplicitlySharedDataPointer<T>::value;


template <typename T, typename = void>
struct is_QSharedDataPointer : std::false_type
{
};

template <typename T>
struct is_QSharedDataPointer<T, decltype(std::declval<typename T::Type>(), void())>
        : std::is_same<T, QSharedDataPointer<typename T::Type>>
{
};

template <class T>
static inline constexpr bool is_QSharedDataPointer_v = is_QSharedDataPointer<T>::value;


template <typename T, typename = void>
struct is_QSharedPointer : std::false_type
{
};

template <typename T>
struct is_QSharedPointer<T, decltype(std::declval<typename T::Type>(), void())>
        : std::is_same<T, QSharedPointer<typename T::Type>>
{
};

template <class T>
static inline constexpr bool is_QSharedPointer_v = is_QSharedPointer<T>::value;


template <typename T, typename = void>
struct is_QWeakPointer : std::false_type
{
};

template <typename T>
struct is_QWeakPointer<T, decltype(void())>
        : std::is_same<T, QWeakPointer<std::remove_pointer_t<decltype(std::declval<T>().data())>>>
{
};

template <class T>
static inline constexpr bool is_QWeakPointer_v = is_QWeakPointer<T>::value;

template <typename T, typename = void>
struct is_QScopedArrayPointer : std::false_type
{
};

template <typename T>
struct is_QScopedArrayPointer<T, decltype(void())>
        : std::is_same<T,
              QScopedArrayPointer<std::remove_pointer_t<decltype(std::declval<T>().data())>>>
{
};

template <class T>
static inline constexpr bool is_QScopedArrayPointer_v = is_QScopedArrayPointer<T>::value;


template <typename T, typename = void>
struct is_QScopedPointer : std::false_type
{
};

template <typename T>
struct is_QScopedPointer<T, decltype(void())>
        : std::is_same<T, QScopedPointer<std::remove_pointer_t<decltype(std::declval<T>().data())>>>
{
};

template <class T>
static inline constexpr bool is_QScopedPointer_v = is_QScopedPointer<T>::value;


template <typename T>
struct is_qt_smart_ptr : std::disjunction<is_QSharedDataPointer<T>,
                             is_QExplicitlySharedDataPointer<T>, is_QSharedPointer<T>,
                             is_QWeakPointer<T>, is_QScopedPointer<T>, is_QScopedArrayPointer<T>>
{
};

template <class T>
static inline constexpr bool is_qt_smart_ptr_v = is_qt_smart_ptr<T>::value;

template <typename T>
struct is_smart_ptr<T, std::enable_if_t<is_qt_smart_ptr_v<T>>> : std::true_type
{
};

}
