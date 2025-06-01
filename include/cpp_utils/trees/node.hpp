#pragma once
/*------------------------------------------------------------------------------ \
 --  This file is a part of cpp_utils                                                         \
 --  Copyright (C) 2019, Plasma Physics Laboratory - CNRS                                     \
 --                                                                                           \
 --  This program is free software; you can redistribute it and/or modify                     \
 --  it under the terms of the GNU General Public License as published by                     \
 --  the Free Software Foundation; either version 2 of the License, or                        \
 --  (at your option) any later version.                                                      \
 --                                                                                           \
 --  This program is distributed in the hope that it will be useful,                          \
 --  but WITHOUT ANY WARRANTY; without even the implied warranty of                           \
 --  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                            \
 --  GNU General Public License for more details.                                             \
 --                                                                                           \
 --  You should have received a copy of the GNU General Public License                        \
 --  along with this program; if not, write to the Free Software                              \
 --  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                \
 -------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
--                            alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#include <vector>
#include "../types/pointers.hpp"

namespace cpp_utils::trees
{
    
    template<typename node_t>
    inline auto&  child(node_t&& node, std::size_t index)
    {
        return cpp_utils::types::pointers::to_value_ref(node.children[index]);
    }
    
    template<typename node_t>
    std::size_t inline children_count(node_t&& node)
    {
        return std::size(node.children);
    }
    
    template<typename node_t>
    auto inline name(node_t&& node)
    {
        return node.name;
    }
    
template <typename string_type = std::string>
struct tree_node
{
    using iterator_t = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().begin());
    using const_iterator_t
        = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().cbegin());

    string_type name;
    tree_node* parent;
    std::vector<std::unique_ptr<tree_node<string_type>>> children;

    tree_node(const string_type& name, tree_node* parent = nullptr)
            : name { name }, parent { parent }
    {
    }

    iterator_t begin() { return children.begin(); }
    iterator_t end() { return children.end(); }
    const_iterator_t begin() const { return children.cbegin(); }
    const_iterator_t end() const { return children.cend(); }
    const_iterator_t cbegin() const { return children.cbegin(); }
    const_iterator_t cend() const { return children.cend(); }
};
}
