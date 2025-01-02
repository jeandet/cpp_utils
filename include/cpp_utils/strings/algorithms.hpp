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
#include "../containers/algorithms.hpp"
#include "../types/strings.hpp"
#include "../types/concepts.hpp"

namespace cpp_utils::strings
{

// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(),
        std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

static inline std::string ltrim(const std::string& s)
{
    auto copy = s;
    copy.erase(copy.begin(),
        std::find_if(copy.begin(), copy.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return copy;
}

static inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
        s.end());
    return s;
}

static inline std::string rtrim(const std::string& s)
{
    auto copy = s;
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
        copy.end());
    return copy;
}

// trim from both ends (in place)
static inline std::string& trim(std::string& s)
{
    return rtrim(ltrim(s));
}

static inline std::string trim(const std::string& s)
{
    return rtrim(ltrim(s));
}

auto make_unique_name(const cpp_utils::types::concepts::contiguous_sequence_container auto& base_name, const cpp_utils::types::concepts::container auto& blacklist)
{
    using string_t = std::decay_t<decltype(base_name)>;
    using namespace cpp_utils;
    if (containers::contains(blacklist, base_name))
    {
        int i = 1;
        string_t name;
        do
        {
            name = base_name + types::strings::to_string<string_t>(i++);
        } while (containers::contains(blacklist, name));
        return name;
    }
    else
    {
        return base_name;
    }
}
}
