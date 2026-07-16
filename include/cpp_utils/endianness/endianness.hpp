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
#include <bit>

inline constexpr bool host_is_big_endian = std::endian::native == std::endian::big;
inline constexpr bool host_is_little_endian = std::endian::native == std::endian::little;

static_assert(host_is_big_endian != host_is_little_endian,
    "cpp_utils::endianness: mixed-endian hosts are not supported");

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

#undef bswap16
#undef bswap32
#undef bswap64

    template <typename T, std::size_t s = sizeof(T)>
    T byte_swap(T value)
    {
        using int_repr_t = uint_t<s>;
        return std::bit_cast<T>(bswap(std::bit_cast<int_repr_t>(value)));
    }

    template <typename T, std::size_t s = sizeof(T)>
    void byte_swap(T* values, std::size_t count)
    {
        std::transform(values, values + count, values, [](const auto& v) { return byte_swap(v); });
    }

    // Reads/writes through memcpy rather than a typed pointer: `data`/`output` may point at an
    // arbitrary byte offset inside a larger buffer (e.g. a struct field mid-way through a wire
    // record) with no alignment guarantee for value_t.
    template <typename value_t>
    inline value_t unaligned_load(const void* data)
    {
        value_t result;
        std::memcpy(&result, data, sizeof(value_t));
        return result;
    }

    template <typename value_t>
    inline void unaligned_store(void* output, const value_t& value)
    {
        std::memcpy(output, &value, sizeof(value_t));
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
        for (auto i = 0UL; i < size; i++)
        {
            const auto swapped
                = details::byte_swap(details::unaligned_load<value_t>(data + i));
            details::unaligned_store(output + i, swapped);
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

// Endianness::unknown defaults to little-endian, matching serde's own default for composites
// that don't opt into big_endian_t.
template <typename T, typename U>
[[nodiscard]] HEDLEY_NON_NULL(2) T decode(Endianness src_endianness, const U* input)
{
    if (src_endianness == Endianness::big)
        return decode<big_endian_t, T>(input);
    return decode<little_endian_t, T>(input);
}

template <typename dst_endianess_t, typename T>
HEDLEY_NON_NULL(2)
inline void encode(T value, char* output)
{
    if constexpr (sizeof(T) > 1)
    {
        if constexpr (!std::is_same_v<host_endianness_t, dst_endianess_t>)
        {
            value = details::byte_swap<T>(value);
        }
        std::memcpy(output, &value, sizeof(T));
    }
    else
    {
        *output = static_cast<char>(value);
    }
}

template <typename T>
HEDLEY_NON_NULL(3)
inline void encode(Endianness dst_endianness, T value, char* output)
{
    if (dst_endianness == Endianness::big)
        encode<big_endian_t>(value, output);
    else
        encode<little_endian_t>(value, output);
}

template <typename dst_endianess_t, typename value_t>
HEDLEY_NON_NULL(1)
inline void encode_v(const value_t* data, std::size_t count, char* output)
{
    if constexpr (sizeof(value_t) > 1 and not std::is_same_v<host_endianness_t, dst_endianess_t>)
    {
        for (auto i = 0UL; i < count; i++)
        {
            const auto swapped = details::byte_swap(data[i]);
            std::memcpy(output + i * sizeof(value_t), &swapped, sizeof(value_t));
        }
    }
    else
    {
        if (count > 0)
            std::memcpy(output, data, count * sizeof(value_t));
    }
}

}
