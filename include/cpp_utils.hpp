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

#include "types/detectors.hpp"
#include "types/pointers.hpp"

#include <functional>
#include <tuple>
#include <type_traits>

// taken from here https://www.fluentcpp.com/2017/10/27/function-aliases-cpp/
#define ALIAS_TEMPLATE_FUNCTION(highLevelF, lowLevelF)                         \
  template<typename... Args>                                                   \
  inline auto highLevelF(Args&&... args)                                       \
      ->decltype(lowLevelF(std::forward<Args>(args)...))                       \
  {                                                                            \
    return lowLevelF(std::forward<Args>(args)...);                             \
  }


template < typename T > constexpr T diff (const std::pair < T, T > &p)
{
  return p.second - p.first;
}



template < typename T > auto repeat_n (T func, int number)->
decltype (func (), void ())
{
  for (int i = 0; i < number; i++)
    func ();
}

template < typename T > auto repeat_n (T func, int number)->
decltype (func (1), void ())
{
  for (int i = 0; i < number; i++)
    func (i);
}



inline int
operator* (int number, const std::function < void (void) > &func)
{
  for (int i = 0; i < number; i++)
    func ();
  return number;
}


