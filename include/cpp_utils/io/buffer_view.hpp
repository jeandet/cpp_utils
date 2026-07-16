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

#include <cstddef>
#include <cstring>
#include <ranges>
#include <span>

#include "../types/concepts.hpp"

namespace cpp_utils::io
{

/** Non-owning, read-only view over an in-memory byte buffer (a vector, array, or another
 * span), providing the same read/view/size/is_valid shape as memory_mapped_file so both can
 * be used interchangeably wherever a random_access_buffer is expected. Ownership of the
 * backing storage stays with the caller, exactly like std::span/std::string_view. */
struct buffer_view
{
    std::span<const char> bytes;

    buffer_view() = default;
    explicit buffer_view(std::span<const char> bytes) : bytes { bytes } { }

    auto begin() const { return bytes.data(); }
    auto end() const { return bytes.data() + bytes.size(); }

    auto view(const std::size_t offset) const { return bytes.data() + offset; }

    void read(char* dest, const std::size_t offset, const std::size_t size) const
    {
        std::memcpy(dest, bytes.data() + offset, size);
    }

    auto data() const { return bytes.data(); }
    std::size_t size() const { return bytes.size(); }

    bool is_valid() const { return bytes.data() != nullptr; }
};

static_assert(std::ranges::contiguous_range<buffer_view>);
static_assert(types::concepts::random_access_buffer<buffer_view>);

}
