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

#include "memory_mapped_file.hpp"
#include "range_reader.hpp"
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace cpp_utils::io
{

// A range_reader over a local file that logs every call, to test and measure what a remote
// reader would be asked for. Copies share the file and the log.
class local_range_reader
{
    struct shared_state
    {
        explicit shared_state(const std::string& path) : file { path } { }
        memory_mapped_file file;
        mutable std::mutex mutex;
        std::vector<std::vector<byte_range>> batches;
        std::vector<std::thread::id> threads;
        std::size_t bytes_served = 0;
    };
    std::shared_ptr<shared_state> state;

    void check_bounds(const byte_range& range) const
    {
        if (range.offset > size() || range.size > size() - range.offset)
            throw std::out_of_range { "range past the end of the file at offset "
                + std::to_string(range.offset) };
    }

    void log(std::span<const byte_range> ranges) const
    {
        std::lock_guard lock { state->mutex };
        state->batches.emplace_back(std::begin(ranges), std::end(ranges));
        state->threads.push_back(std::this_thread::get_id());
        for (const auto& range : ranges)
            state->bytes_served += range.size;
    }

public:
    explicit local_range_reader(const std::string& path)
            : state { std::make_shared<shared_state>(path) }
    {
    }

    [[nodiscard]] std::size_t size() const { return state->file.size(); }

    void read_ranges(std::span<const byte_range> ranges, std::span<char* const> destinations) const
    {
        for (const auto& range : ranges)
            check_bounds(range);
        log(ranges);
        for (std::size_t i = 0; i < std::size(ranges); ++i)
            std::memcpy(destinations[i], state->file.data() + ranges[i].offset, ranges[i].size);
    }

    [[nodiscard]] std::vector<std::vector<byte_range>> batches() const
    {
        std::lock_guard lock { state->mutex };
        return state->batches;
    }

    [[nodiscard]] std::size_t bytes_served() const
    {
        std::lock_guard lock { state->mutex };
        return state->bytes_served;
    }

    [[nodiscard]] std::vector<std::thread::id> caller_threads() const
    {
        std::lock_guard lock { state->mutex };
        return state->threads;
    }
};

}
