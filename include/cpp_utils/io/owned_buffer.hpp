/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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

#include "../containers/no_init_vector.hpp"
#include "../types/concepts.hpp"
#include <cstddef>
#include <cstring>
#include <ranges>
#include <utility>
#include <vector>

namespace cpp_utils::io
{

/** Owning counterpart to buffer_view: holds an arbitrary contiguous byte container by
 * value and exposes the same random_access_buffer shape (read/view/size/is_valid), for
 * buffers whose lifetime must outlive whatever produced them — e.g. a freshly-
 * decompressed payload that has nowhere else to live, or a caller-supplied
 * std::vector<char> whose ownership is being transferred in. Templated on Storage so a
 * true move is possible regardless of the caller's own container type — no forced
 * conversion into no_init_vector, no copy. */
template <typename Storage = containers::no_init_vector<char>>
struct owned_buffer_t
{
    Storage storage;

    owned_buffer_t() = default;
    explicit owned_buffer_t(Storage&& storage) : storage { std::move(storage) } { }

    auto begin() const { return storage.data(); }
    auto end() const { return storage.data() + storage.size(); }

    auto view(const std::size_t offset) const { return storage.data() + offset; }

    void read(char* dest, const std::size_t offset, const std::size_t size) const
    {
        std::memcpy(dest, storage.data() + offset, size);
    }

    auto data() const { return storage.data(); }
    std::size_t size() const { return storage.size(); }

    bool is_valid() const { return storage.data() != nullptr; }
};

using owned_buffer = owned_buffer_t<containers::no_init_vector<char>>;

static_assert(std::ranges::contiguous_range<owned_buffer>);
static_assert(types::concepts::random_access_buffer<owned_buffer>);
static_assert(types::concepts::random_access_buffer<owned_buffer_t<std::vector<char>>>);

}
