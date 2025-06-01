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

#include <string>
#include "../types/detectors.hpp"

namespace cpp_utils::trees
{
using namespace cpp_utils::types::detectors;




namespace
{
    HAS_MEMBER(name)
    HAS_METHOD(has_name_method, name)
    HAS_METHOD(has_text_method, text)
}

template <typename T>
std::string _get_name(const T& item)
{
    if constexpr (has_name_method_v<T>)
    {
        return _get_name(item.name());
    }
    else if constexpr (has_text_method_v<T>)
    {
        return _get_name(item.text(0));
    }
    else if constexpr (has_toStdString_method_v<T>)
    {
        return _get_name(item.toStdString());
    }
    else if constexpr (has_name_member_object_v<T>)
    {
        return std::to_string(item.name);
    }
    else
    {
        return item;
    }
}



}
