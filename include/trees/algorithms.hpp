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

#include "../cpp_utils.hpp"
#include "../trees/node.hpp"
#include "../trees/traits.hpp"
#include "../types/detectors.hpp"
#include "../types/pointers.hpp"
#include "../types/strings.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace cpp_utils::trees
{

namespace details
{
    template <typename T, typename U>
    void _print_tree(T&& tree, int indent_increment, int indent_lvl, U& ostream)
    {
        using namespace cpp_utils::types::strings;
        using namespace cpp_utils::types::pointers;
        auto& node = to_value_ref(tree);
        for (std::size_t i = 0; i < children_count(node); i++)
        {
            auto& child_node = child(node, i);

            repeat_n([indent_increment,
                         &ostream]() { ostream << "│" << std::string(indent_increment, ' '); },
                indent_lvl);
            ostream << "├";
            repeat_n([&ostream]() { ostream << "─"; }, indent_increment - 1);
            ostream << " " << std::to_string(name(child_node)) << std::endl;
            _print_tree(child_node, indent_increment, indent_lvl + 1, ostream);
        }
    }
}


template <typename T, typename U = decltype(std::cout), int indent_increment = 3>
void print_tree(T&& tree, U& ostream = std::cout)
{
    using namespace cpp_utils::types::strings;
    using namespace cpp_utils::types::pointers;
    ostream << std::to_string(name(to_value_ref(tree))) << std::endl;
    details::_print_tree(to_value_ref(tree), indent_increment, 0, ostream);
}

}
