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

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace cpp_utils::types
{

// Dispatches on a Python buffer-protocol / struct-module format code, calling
// func(std::type_identity<T>{}) for the matching T. Every branch of func must
// return the same type.
template <typename F>
auto dispatch_dtype(char format_code, F&& func)
{
    switch (format_code)
    {
        case 'f': return func(std::type_identity<float> {});
        case 'd': return func(std::type_identity<double> {});
        case 'b': return func(std::type_identity<int8_t> {});
        case 'B': return func(std::type_identity<uint8_t> {});
        case 'h': return func(std::type_identity<int16_t> {});
        case 'H': return func(std::type_identity<uint16_t> {});
        case 'i': return func(std::type_identity<int32_t> {});
        case 'I': return func(std::type_identity<uint32_t> {});
        case 'l': return func(std::type_identity<long> {});
        case 'L': return func(std::type_identity<unsigned long> {});
        case 'q': return func(std::type_identity<long long> {});
        case 'Q': return func(std::type_identity<unsigned long long> {});
        default:
            throw std::invalid_argument(
                std::string("Unsupported dtype format code: '") + format_code + "'");
    }
}

}
