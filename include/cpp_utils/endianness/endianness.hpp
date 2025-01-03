#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2020, Plasma Physics Laboratory - CNRS
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
#include "config.h"
#ifdef CPP_UTILS_BIG_ENDIAN
inline const bool host_is_big_endian = true;
inline const bool host_is_little_endian = false;
#else
#ifdef CPP_UTILS_LITTLE_ENDIAN
inline const bool host_is_big_endian = false;
inline const bool host_is_little_endian = true;
#else
#error "Can't find if platform is either big or little endian"
#endif
#endif

#ifdef __GNUC__
#define bswap16 __builtin_bswap16
#define bswap32 __builtin_bswap32
#define bswap64 __builtin_bswap64
#endif

#ifdef _MSC_VER
#include <stdlib.h>
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x) _byteswap_ulong(x)
#define bswap64(x) _byteswap_uint64(x)
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include <hedley.h>

namespace cpp_utils::endianness
{
namespace details
{
    template <std::size_t s>
    struct uint;

    template <>
    struct uint<1>
    {
        using type = uint8_t;
    };
    template <>
    struct uint<2>
    {
        using type = uint16_t;
    };
    template <>
    struct uint<4>
    {
        using type = uint32_t;
    };
    template <>
    struct uint<8>
    {
        using type = uint64_t;
    };

    template <std::size_t s>
    using uint_t = typename uint<s>::type;


    inline uint8_t bswap(uint8_t v)
    {
        return v;
    }
    inline uint16_t bswap(uint16_t v)
    {
        return bswap16(v);
    }
    inline uint32_t bswap(uint32_t v)
    {
        return bswap32(v);
    }
    inline uint64_t bswap(uint64_t v)
    {
        return bswap64(v);
    }

    template <typename T, std::size_t s = sizeof(T)>
    T byte_swap(T value)
    {
        using int_repr_t = uint_t<s>;
        int_repr_t result = bswap(*reinterpret_cast<int_repr_t*>(&value));
        return *reinterpret_cast<T*>(&result);
    }

    template <typename T, std::size_t s = sizeof(T)>
    void byte_swap(T* values, std::size_t count)
    {
        std::transform(values, values + count, values, [](const auto& v) { return byte_swap(v); });
    }
}

enum class Endianness
{
    unknown,
    big,
    little
};

struct big_endian_t
{
};

struct little_endian_t
{
};

using host_endianness_t = std::conditional_t<host_is_little_endian, little_endian_t, big_endian_t>;

template <typename endianness_t>
inline constexpr bool is_little_endian_v = std::is_same_v<little_endian_t, endianness_t>;

template <typename endianness_t>
inline constexpr bool is_big_endian_v = std::is_same_v<big_endian_t, endianness_t>;

inline Endianness host_endianness_v()
{
    if constexpr (is_little_endian_v<host_endianness_t>)
        return Endianness::little;
    if constexpr (is_big_endian_v<host_endianness_t>)
        return Endianness::big;
    return Endianness::unknown;
}


template <typename src_endianess_t, typename T, typename U>
[[nodiscard]] HEDLEY_NON_NULL(1) T decode(const U* input)
{
    if constexpr (sizeof(T) > 1)
    {
        T result;
        std::memcpy(&result, input, sizeof(T));
        if constexpr (!std::is_same_v<host_endianness_t, src_endianess_t>)
        {
            return details::byte_swap<T>(result);
        }
        return result;
    }
    else
    {
        return static_cast<T>(*input);
    }
}

template <typename src_endianess_t, typename value_t>
HEDLEY_NON_NULL(1)
inline void _impl_decode_v(const value_t* data, std::size_t size, value_t* output)
{
    if constexpr (sizeof(value_t) > 1 and not std::is_same_v<host_endianness_t, src_endianess_t>)
    {
        if (size > 0)
        {
            for (auto i = 0UL; i < size; i++)
            {
                output[i] = details::byte_swap(data[i]);
            }
        }
    }
    else
    {
        if (size > 0 && data != output)
        {
            std::memcpy(output, data, size * sizeof(value_t));
        }
    }
}


template <typename src_endianess_t, typename value_t>
HEDLEY_NON_NULL(1)
inline void decode_v(value_t* data, std::size_t size)
{
    _impl_decode_v<src_endianess_t>(reinterpret_cast<details::uint_t<sizeof(value_t)>*>(data), size,
        reinterpret_cast<details::uint_t<sizeof(value_t)>*>(data));
}

template <typename src_endianess_t, typename value_t>
HEDLEY_NON_NULL(1)
inline void decode_v(const auto* data, std::size_t size, value_t* output)
{
    using _value_t = details::uint_t<sizeof(value_t)>;
    auto count = size * sizeof(decltype(*data)) / sizeof(value_t);
    _impl_decode_v<src_endianess_t>(
        reinterpret_cast<const _value_t*>(data), count, reinterpret_cast<_value_t*>(output));
}

}
