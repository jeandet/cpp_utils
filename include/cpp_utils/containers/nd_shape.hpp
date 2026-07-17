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

#include "algorithms.hpp"

#include <cstddef>
#include <vector>

namespace cpp_utils::containers
{

enum class array_order
{
    row_major, // C order: last dimension varies fastest
    column_major // Fortran order: first dimension varies fastest
};

// Flat offset of an N-D `indices` point into an array of the given per-dimension
// `shape`, for arbitrary dimension count (a plain O(ndim) loop - no compile-time
// cap on the number of dimensions).
template <typename IndexRange, typename ShapeRange>
std::size_t flat_index(
    const IndexRange& indices, const ShapeRange& shape, array_order order = array_order::row_major)
{
    std::size_t result = 0;
    const std::size_t ndim = std::size(shape);
    if (order == array_order::row_major)
    {
        for (std::size_t dim = 0; dim < ndim; ++dim)
            result = result * static_cast<std::size_t>(shape[dim])
                + static_cast<std::size_t>(indices[dim]);
    }
    else
    {
        for (std::size_t dim = ndim; dim-- > 0;)
            result = result * static_cast<std::size_t>(shape[dim])
                + static_cast<std::size_t>(indices[dim]);
    }
    return result;
}

// Reorders a row-major N-D array's flat buffer so that axis `permutation[k]` of
// `data`/`shape` becomes axis `k` of the result (a generalization of a full-axis
// reversal transpose to an arbitrary axis permutation). `permutation` must be a
// permutation of [0, shape.size()).
//
// This favors genericity/clarity over raw throughput (one element at a time,
// no block-copy fast path for contiguous runs) - callers with a hot,
// fixed-permutation path should special-case it themselves.
template <typename Container>
Container permute_axes(const Container& data, const std::vector<std::size_t>& shape,
    const std::vector<std::size_t>& permutation)
{
    const std::size_t ndim = std::size(shape);
    std::vector<std::size_t> new_shape(ndim);
    for (std::size_t k = 0; k < ndim; ++k)
        new_shape[k] = shape[permutation[k]];

    Container result(flat_size(shape));
    std::vector<std::size_t> index(ndim, 0);
    const std::size_t total = flat_size(shape);
    for (std::size_t flat = 0; flat < total; ++flat)
    {
        std::vector<std::size_t> new_index(ndim);
        for (std::size_t k = 0; k < ndim; ++k)
            new_index[k] = index[permutation[k]];
        result[flat_index(new_index, new_shape)] = data[flat];

        for (std::size_t dim = ndim; dim-- > 0;)
        {
            if (++index[dim] < shape[dim])
                break;
            index[dim] = 0;
        }
    }
    return result;
}

}
