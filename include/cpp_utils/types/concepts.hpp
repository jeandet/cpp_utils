#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2024, Plasma Physics Laboratory - CNRS
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
#if __cplusplus >= 202000
#include <concepts>
#define CPP_UTILS_CONCEPTS_SUPPORTED
#endif
namespace cpp_utils::types::concepts
{
#ifdef CPP_UTILS_CONCEPTS_SUPPORTED

/** pointer_to_contiguous_memory concept, requires pointer arithmetic and dereference */
template <class T>
concept pointer_to_contiguous_memory = requires(T t) {
    { *t } -> std::convertible_to<std::remove_pointer_t<T>>;
    { t + 1 } -> std::same_as<T>;
    { t - 1 } -> std::same_as<T>;
    { t += 1 } -> std::same_as<T&>;
    { t -= 1 } -> std::same_as<T&>;
    { t[0] } -> std::convertible_to<std::remove_pointer_t<T>>;
};

template <class T>
concept fundamental_type = std::is_fundamental_v<std::decay_t<T>>;

#else
#warning "Concepts not supported before C++20"
#endif
} // namespace cpp_utils::types::concepts
