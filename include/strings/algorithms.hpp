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
#include "../containers/algorithms.hpp"
#include "../types/strings.hpp"

namespace cpp_utils::strings
{
template <typename string_t, template <typename val_t, typename...> class list_t>
auto make_unique_name(const string_t& base_name, const list_t<string_t>& blacklist)
{
    using namespace cpp_utils;
    if (containers::contains(blacklist, base_name))
    {
        int i = 1;
        string_t name;
        do
        {
            name = base_name + types::strings::to_string<string_t>(i++);
        } while (containers::contains(blacklist, name));
        return name;
    }
    else
    {
        return base_name;
    }
}
}
