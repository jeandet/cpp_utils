/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the “Software”), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#pragma once
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

#include "cpp_utils/containers/no_init_vector.hpp"

// Numpy helpers shared by the CDF and CEF python wrappers: allocate without zero-filling what the
// parser is about to overwrite, and expose C++ buffers as zero-copy numpy views that keep their
// producer alive. Opt-in by include: only code including this header needs pybind11.
namespace cpp_utils::python
{
template <typename Shape>
[[nodiscard]] inline std::size_t flat_size(const Shape& shape)
{
    return std::accumulate(std::cbegin(shape), std::cend(shape), std::size_t { 1 },
        std::multiplies<std::size_t>());
}

// numpy keeps the capsule alive as long as any array built on it exists, so the shared owner
// outlives the C++ scope that produced the buffer.
[[nodiscard]] inline pybind11::capsule owner_capsule(std::shared_ptr<const void> owner)
{
    auto* owned = new std::shared_ptr<const void>(std::move(owner));
    return pybind11::capsule(owned, [](void* ptr) {
        delete static_cast<std::shared_ptr<const void>*>(ptr);
    });
}

template <typename T, typename Shape, typename Owner = std::nullptr_t>
[[nodiscard]] inline auto fast_allocate_array(const Shape& shape, Owner&& owner = std::nullptr_t {})
{
    using value_t = std::remove_const_t<T>;
    using allocator_t = containers::default_init_allocator<value_t>;

    if (flat_size(shape) == 0)
        return pybind11::array_t<value_t>(shape);

    auto* ptr = allocator_t {}.allocate(flat_size(shape));

    if constexpr (std::is_same_v<std::nullptr_t, std::remove_cvref_t<Owner>>)
        return pybind11::array_t<value_t>(shape, ptr, pybind11::capsule(ptr, [](void* raw) {
            allocator_t {}.deallocate(static_cast<value_t*>(raw), 0);
        }));
    else
        return pybind11::array_t<value_t>(shape, ptr, std::forward<Owner>(owner));
}

template <typename T>
[[nodiscard]] inline pybind11::array_t<std::remove_const_t<T>> view_with_owner(
    T* data, const std::vector<pybind11::ssize_t>& shape, std::shared_ptr<const void> owner)
{
    using value_t = std::remove_const_t<T>;

    if (flat_size(shape) == 0)
        return pybind11::array_t<value_t>(shape);

    return pybind11::array_t<value_t>(
        shape, const_cast<value_t*>(data), owner_capsule(std::move(owner)));
}
}
