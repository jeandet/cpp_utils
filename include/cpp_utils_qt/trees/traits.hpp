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

#include "../../trees/node.hpp"
#if __has_include(<QTreeWidgetItem>)
#include <QTreeWidgetItem>
namespace cpp_utils::trees
{
template <>
inline auto& child<QTreeWidgetItem&>(QTreeWidgetItem& node, int index)
{
    return *node.child(index);
}

template <>
inline auto& child<QTreeWidgetItem&&>(QTreeWidgetItem&& node, int index)
{
    return *node.child(index);
}

template <>
std::size_t inline children_count<QTreeWidgetItem&>(QTreeWidgetItem& node)
{
    return node.childCount();
}

template <>
std::size_t inline children_count<QTreeWidgetItem&&>(QTreeWidgetItem&& node)
{
    return node.childCount();
}

template <>
auto inline name<QTreeWidgetItem&>(QTreeWidgetItem& node)
{
    return node.text(0);
}

template <>
auto inline name<QTreeWidgetItem&&>(QTreeWidgetItem&& node)
{
    return node.text(0);
}

}
#endif
