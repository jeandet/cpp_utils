#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2026, Plasma Physics Laboratory - CNRS
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

#include "../containers/traits.hpp"
#include <string>
#include <string_view>
#include <type_traits>

namespace cpp_utils::strings
{

template <typename T>
struct is_string_like
        : std::disjunction<containers::is_std_basic_string<std::decay_t<T>>,
              std::is_same<std::decay_t<T>, std::string_view>,
              std::is_same<std::decay_t<T>, const char*>, std::is_same<std::decay_t<T>, char*>>
{
};

template <typename T>
static inline constexpr bool is_string_like_v = is_string_like<T>::value;

}
