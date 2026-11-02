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

#include "range_reader.hpp"
#include "reserved_region.hpp"
#include <algorithm>
#include <charconv>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cpp_utils::io
{

struct remote_buffer_options
{
    std::size_t read_ahead = 256 * 1024;
    std::size_t tail_prefetch = 256 * 1024;
    region_backing backing = region_backing::memory;
    std::filesystem::path temp_dir {};
    std::size_t download_whole_below = 0;

    // Reads <prefix>READ_AHEAD, TAIL_PREFETCH, BACKING (memory|file), TEMP_DIR and
    // DOWNLOAD_WHOLE_BELOW; unset or empty variables keep the given defaults.
    [[nodiscard]] static remote_buffer_options from_env(
        std::string_view prefix, remote_buffer_options defaults);
    [[nodiscard]] static remote_buffer_options from_env(std::string_view prefix);

private:
    static std::string variable_name(std::string_view prefix, std::string_view name)
    {
        return std::string { prefix } + std::string { name };
    }

    static std::string env(std::string_view prefix, std::string_view name)
    {
        const char* value = std::getenv(variable_name(prefix, name).c_str());
        return value ? std::string { value } : std::string {};
    }

    static void read_size(std::string_view prefix, std::string_view name, std::size_t& target)
    {
        const auto value = env(prefix, name);
        if (value.empty())
            return;
        std::size_t parsed = 0;
        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (error != std::errc {} || end != value.data() + value.size())
            throw std::invalid_argument { variable_name(prefix, name) + ": not a byte count: "
                + value };
        target = parsed;
    }

    static region_backing parse_backing(const std::string& name, const std::string& value)
    {
        if (value == "memory")
            return region_backing::memory;
        if (value == "file")
            return region_backing::file;
        throw std::invalid_argument { name + ": expected memory or file, got " + value };
    }
};

inline remote_buffer_options remote_buffer_options::from_env(
    std::string_view prefix, remote_buffer_options defaults)
{
    read_size(prefix, "READ_AHEAD", defaults.read_ahead);
    read_size(prefix, "TAIL_PREFETCH", defaults.tail_prefetch);
    read_size(prefix, "DOWNLOAD_WHOLE_BELOW", defaults.download_whole_below);
    if (const auto value = env(prefix, "TEMP_DIR"); !value.empty())
        defaults.temp_dir = value;
    if (const auto value = env(prefix, "BACKING"); !value.empty())
        defaults.backing = parse_backing(variable_name(prefix, "BACKING"), value);
    return defaults;
}

inline remote_buffer_options remote_buffer_options::from_env(std::string_view prefix)
{
    return from_env(prefix, remote_buffer_options {});
}

// Lazily fetched copy of a range_reader's bytes, readable in place through data(). ensure()
// fetches what is missing, widened to the read-ahead window, in one batch; each byte is fetched
// at most once. Bytes never fetched are not readable: check_present() throws, and a raw read
// hits a no-access page.
template <range_reader R>
class remote_buffer
{
    enum class page_state : std::uint8_t
    {
        absent,
        in_flight,
        present
    };

    struct page_run
    {
        std::size_t first;
        std::size_t count;
    };

    struct fetch_plan
    {
        std::vector<page_run> runs;
        std::vector<std::size_t> awaited;
        bool includes_first_extras = false;
    };

    R p_reader;
    remote_buffer_options p_options;
    std::size_t p_size;
    mutable reserved_region p_region;
    mutable std::mutex p_mutex;
    mutable std::condition_variable p_page_changed;
    mutable std::vector<page_state> p_pages;
    mutable bool p_first_fetch_planned = false;

    static std::size_t round_up(std::size_t value, std::size_t step)
    {
        return (value + step - 1) / step * step;
    }

    static remote_buffer_options normalized(remote_buffer_options options)
    {
        if (options.read_ahead > std::numeric_limits<std::size_t>::max() / 2)
            throw std::invalid_argument { "read_ahead is too large" };
        options.read_ahead = std::max(page_size(), round_up(options.read_ahead, page_size()));
        return options;
    }

    void check_bounds(const byte_range& range) const
    {
        if (range.offset > p_size || range.size > p_size - range.offset)
            throw std::runtime_error { "byte range outside the remote file at offset "
                + std::to_string(range.offset) };
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> widened_pages(const byte_range& range) const
    {
        const auto window = p_options.read_ahead;
        const auto begin = range.offset / window * window;
        const auto end = std::min(round_up(range.offset + range.size, window), p_size);
        return { begin / page_size(), round_up(end, page_size()) / page_size() };
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> exact_pages(const byte_range& range) const
    {
        return { range.offset / page_size(),
            round_up(range.offset + range.size, page_size()) / page_size() };
    }

    [[nodiscard]] std::vector<byte_range> first_fetch_extras() const
    {
        if (p_size < p_options.download_whole_below)
            return { { 0, p_size } };
        const auto tail = std::min(p_options.tail_prefetch, p_size);
        if (tail == 0)
            return {};
        return { { p_size - tail, tail } };
    }

    // Called with the lock held. Everything that can throw (allocations) happens before the
    // first page changes state, so a failure here leaves no page claimed.
    [[nodiscard]] fetch_plan plan(std::span<const byte_range> ranges) const
    {
        fetch_plan result;
        std::vector<bool> claimed(std::size(p_pages), false);
        result.includes_first_extras = !p_first_fetch_planned;
        if (result.includes_first_extras)
            for (const auto& extra : first_fetch_extras())
                select_absent(exact_pages(extra), claimed);
        for (const auto& range : ranges)
            select_absent(widened_pages(range), claimed);
        for (const auto& range : ranges)
        {
            const auto [first, last] = exact_pages(range);
            for (auto page = first; page < last; ++page)
                if (p_pages[page] == page_state::in_flight && !claimed[page])
                    result.awaited.push_back(page);
        }
        result.runs = runs_of(claimed);
        set_state(result.runs, page_state::in_flight);
        p_first_fetch_planned = true;
        return result;
    }

    void select_absent(std::pair<std::size_t, std::size_t> pages, std::vector<bool>& claimed) const
    {
        for (auto page = pages.first; page < pages.second; ++page)
            claimed[page] = claimed[page] || p_pages[page] == page_state::absent;
    }

    void set_state(const std::vector<page_run>& runs, page_state state) const noexcept
    {
        for (const auto& run : runs)
            std::fill_n(std::begin(p_pages) + run.first, run.count, state);
    }

    static std::vector<page_run> runs_of(const std::vector<bool>& claimed)
    {
        std::vector<page_run> runs;
        for (std::size_t page = 0; page < std::size(claimed); ++page)
        {
            if (!claimed[page])
                continue;
            if (!runs.empty() && runs.back().first + runs.back().count == page)
                ++runs.back().count;
            else
                runs.push_back({ page, 1 });
        }
        return runs;
    }

    [[nodiscard]] byte_range bytes_of(const page_run& run) const
    {
        const auto offset = run.first * page_size();
        return { offset, std::min(run.count * page_size(), p_size - offset) };
    }

    void publish(const fetch_plan& plan, page_state state) const
    {
        {
            std::lock_guard lock { p_mutex };
            set_state(plan.runs, state);
            if (state == page_state::absent && plan.includes_first_extras)
                p_first_fetch_planned = false;
        }
        p_page_changed.notify_all();
    }

    // A page left readable after a failed fetch would serve partial bytes: if it cannot be
    // closed again, crashing (noexcept) is the safe outcome.
    void close_pages(std::span<const page_run> runs) const noexcept
    {
        for (const auto& run : runs)
            p_region.make_no_access(run.first, run.count);
    }

    void fetch(const fetch_plan& plan) const
    {
        std::vector<byte_range> ranges;
        std::vector<char*> destinations;
        std::size_t opened = 0;
        try
        {
            for (const auto& run : plan.runs)
            {
                ranges.push_back(bytes_of(run));
                destinations.push_back(p_region.make_writable(run.first, run.count));
                ++opened;
            }
            p_reader.read_ranges(ranges, destinations);
            for (const auto& run : plan.runs)
                p_region.seal(run.first, run.count);
        }
        catch (...)
        {
            close_pages(std::span { plan.runs }.first(opened));
            publish(plan, page_state::absent);
            throw;
        }
        publish(plan, page_state::present);
    }

    // Readers whose callers hold an outer lock (Python's GIL) provide blocking_wait(f), to
    // release it while f blocks: the fetching thread may need it.
    template <typename F>
    void blocking_wait(F&& wait) const
    {
        if constexpr (requires { p_reader.blocking_wait(wait); })
            p_reader.blocking_wait(wait);
        else
            wait();
    }

    // True when every awaited page arrived; false when one of the fetches failed.
    [[nodiscard]] bool wait_for(const std::vector<std::size_t>& pages) const
    {
        if (pages.empty())
            return true;
        std::unique_lock lock { p_mutex, std::defer_lock };
        blocking_wait(
            [&]
            {
                lock.lock();
                p_page_changed.wait(lock,
                    [&]
                    {
                        return std::ranges::none_of(pages,
                            [&](auto page) { return p_pages[page] == page_state::in_flight; });
                    });
            });
        return std::ranges::all_of(
            pages, [&](auto page) { return p_pages[page] == page_state::present; });
    }

public:
    explicit remote_buffer(R reader, remote_buffer_options options = {})
            : p_reader { std::move(reader) }
            , p_options { normalized(std::move(options)) }
            , p_size { p_reader.size() }
            , p_region { p_size, p_options.backing, p_options.temp_dir }
            , p_pages(round_up(p_size, page_size()) / page_size(), page_state::absent)
    {
    }

    void ensure(std::span<const byte_range> ranges) const
    {
        std::vector<byte_range> wanted;
        for (const auto& range : ranges)
        {
            check_bounds(range);
            if (range.size != 0)
                wanted.push_back(range);
        }
        while (!wanted.empty())
        {
            fetch_plan next;
            {
                std::lock_guard lock { p_mutex };
                next = plan(wanted);
            }
            if (!next.runs.empty())
                fetch(next);
            if (wait_for(next.awaited))
                return;
        }
    }

    void ensure(std::size_t offset, std::size_t size) const
    {
        const byte_range range { offset, size };
        ensure(std::span { &range, 1 });
    }

    void check_present(std::size_t offset, std::size_t size) const
    {
        check_bounds({ offset, size });
        const auto [first, last] = exact_pages({ offset, size });
        std::lock_guard lock { p_mutex };
        for (auto page = first; page < last; ++page)
            if (p_pages[page] != page_state::present)
                throw std::runtime_error { "remote bytes not fetched at offset "
                    + std::to_string(std::max(offset, page * page_size())) };
    }

    void read(char* dest, std::size_t offset, std::size_t size) const
    {
        if (size == 0)
            return;
        ensure(offset, size);
        std::memcpy(dest, p_region.data() + offset, size);
    }

    [[nodiscard]] const char* data() const { return p_region.data(); }
    [[nodiscard]] const char* begin() const { return p_region.data(); }
    [[nodiscard]] const char* end() const { return p_region.data() + p_size; }
    [[nodiscard]] std::size_t size() const { return p_size; }
    [[nodiscard]] bool is_valid() const { return p_size != 0; }
    [[nodiscard]] const R& reader() const { return p_reader; }
};

}
