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

// Requires the bshoshany-thread-pool dependency, deliberately kept out of
// cpp_utils_dep (see cpp_utils_qt's rationale in CLAUDE.md) so plain consumers
// never pull it in: depend on it yourself if you use this header.
#include <BS_thread_pool.hpp>

#include <cstddef>
#include <string>
#include <utility>

#ifdef __linux__
#include <pthread.h>
#endif

namespace cpp_utils::threading
{

namespace details
{
    inline void name_pool_thread(std::size_t index) noexcept
    {
#ifdef __linux__
        // 15-char OS limit (TASK_COMM_LEN=16 incl. null); "cpu" + index fits
        // comfortably for any realistic thread count.
        ::pthread_setname_np(::pthread_self(), ("cpu" + std::to_string(index)).c_str());
#else
        (void)index;
#endif
    }
}

// Process-wide thread pool, lazily initialized on first use, sized to
// hardware_concurrency.
inline BS::light_thread_pool& pool()
{
    static BS::light_thread_pool instance { &details::name_pool_thread };
    return instance;
}

// Calls f(index) for every index in [0, count), distributed across pool().
// Blocks until all calls complete. Runs inline (no pool involvement) for
// count <= 1.
template <typename F>
void parallel_for(std::size_t count, F&& f)
{
    if (count == 0)
        return;
    if (count == 1)
    {
        f(std::size_t { 0 });
        return;
    }
    pool().detach_sequence(std::size_t { 0 }, count, std::forward<F>(f));
    pool().wait();
}

// Convenience: parallel_for over a container's elements by reference.
template <typename Container, typename F>
void parallel_for_each(Container& items, F&& f)
{
    parallel_for(std::size(items), [&](std::size_t i) { f(items[i]); });
}

}
