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

#include "../containers/traits.hpp"
#include "../cpp_utils.hpp"
#include "../types/detectors.hpp"
#include <algorithm>
#include <string>

namespace cpp_utils::containers
{
template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.front(), std::end(container), true)
{
    static_assert(is_container_v<T1>, "");
    return std::find(std::cbegin(container), std::cend(container), value) != std::cend(container);
}

template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.find(value), std::cend(container), true)
{
    static_assert(is_container_v<T1>, "");
    return container.find(value) != std::cend(container);
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value) -> decltype(0)
{
    static_assert(is_sequence_container_v<T1>, "");
    return std::distance(
        std::cbegin(container), std::find(std::cbegin(container), std::cend(container), value));
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value)
    -> decltype(container.front().get(), std::is_pointer<T2>::value, 0)
{
    static_assert(is_container_v<T1>, "");
    return std::distance(std::cbegin(container),
        std::find_if(std::cbegin(container), std::cend(container),
            [value](const auto& item) { return value == item.get(); }));
}

template <class input_t, template <typename val_t, typename...> class output_t, typename value_t>
auto split(const input_t& input, output_t<input_t>& output, const value_t& splitVal)
    -> decltype(void())
{
    static_assert(is_container_v<input_t>, "");
    static_assert(is_container_v<output_t<input_t>>, "");
    if (std::size(input) == 0)
        return;
    auto current = std::cbegin(input);
    auto end = std::cend(input);
    input_t chunk;
    while (current != end)
    {
        if (*current != splitVal)
            chunk.push_back(*current);
        else
        {
            if (std::size(chunk))
            {
                output.push_back(std::move(chunk));
                chunk = input_t {};
            }
        }
        current++;
    }
    if (std::size(chunk))
        output.push_back(std::move(chunk));
}

template <class input_t, class item_t>
auto join(const input_t& input, const item_t& sep)
{
    std::remove_cv_t<std::remove_reference_t<decltype(input.front())>> result;
    if (std::size(input))
    {
        auto result_size = std::accumulate(std::cbegin(input), std::cend(input), 0UL,
            [](const auto& size, const auto& item) { return size + std::size(item); });
        result_size += std::size(input) - 1;
        result.reserve(result_size);
        if (std::size(input) > 1)
        {
            std::for_each(
                std::cbegin(input), --std::cend(input), [&result, &sep](const auto& item) {
                    std::copy(std::cbegin(item), std::cend(item), std::back_inserter(result));
                    result.push_back(sep);
                });
        }
        std::copy(std::cbegin(input.back()), std::cend(input.back()), std::back_inserter(result));
    }
    return result;
}

} //
