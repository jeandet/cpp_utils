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
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace cpp_utils::io
{

/** sequential_writer concept: an append-only byte sink that tracks its own write
 * cursor. write() appends `count` bytes and returns the new cursor position, fill()
 * appends `count` copies of a byte the same way, offset() reports the current
 * cursor. Satisfied by vector_writer and file_writer — the write-side counterpart to
 * random_access_buffer on the read side. */
template <typename T>
concept sequential_writer = requires(T t, const char* p, std::size_t n, char v) {
    { t.write(p, n) } -> std::convertible_to<std::size_t>;
    { t.fill(v, n) } -> std::convertible_to<std::size_t>;
    { t.offset() } -> std::convertible_to<std::size_t>;
};

/** Appends into any in-memory, resizable byte container (a std::vector<char>, a
 * no_init_vector<char>, or a cpp_utils::serde dynamic_array_bytes field) that grows
 * as bytes are written. Non-owning: the container's lifetime is the caller's
 * responsibility. */
template <typename Container>
struct vector_writer
{
    Container& data;
    std::size_t global_offset;

    explicit vector_writer(Container& data) : data { data }, global_offset { 0 } { }

    std::size_t write(const char* const data_ptr, std::size_t count)
    {
        data.resize(global_offset + count);
        std::memcpy(data.data() + global_offset, data_ptr, count);
        global_offset += count;
        return global_offset;
    }

    std::size_t fill(const char v, std::size_t count)
    {
        data.resize(global_offset + count);
        std::memset(data.data() + global_offset, v, count);
        global_offset += count;
        return global_offset;
    }

    [[nodiscard]] std::size_t offset() const noexcept { return this->global_offset; }
};

template <typename Container>
vector_writer(Container&) -> vector_writer<Container>;

/** Streams bytes straight to an fstream without materializing the output in memory
 * first — needed for bounded-memory saves of potentially gigabyte-scale files. */
struct file_writer
{
    std::fstream os;
    std::size_t global_offset;

    explicit file_writer(const std::string& fname) : global_offset { 0 }
    {
        this->os
            = std::fstream(fname, std::fstream::out | std::fstream::binary | std::fstream::trunc);
    }
    ~file_writer()
    {
        if (is_open())
        {
            os.flush();
            os.close();
        }
    }

    [[nodiscard]] bool is_open() const noexcept { return os.is_open(); }

    std::size_t write(const char* const data_ptr, std::size_t count)
    {
        os.write(data_ptr, count);
        global_offset += count;
        return global_offset;
    }

    std::size_t fill(const char v, std::size_t count)
    {
        std::vector<char> values(count, v);
        os.write(values.data(), count);
        global_offset += count;
        return global_offset;
    }

    [[nodiscard]] std::size_t offset() const noexcept { return this->global_offset; }
};

}
