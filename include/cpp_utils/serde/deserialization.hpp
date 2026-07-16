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
#include "../endianness/endianness.hpp"
#include "../reflexion/reflection.hpp"
#include "../types/concepts.hpp"
#include "context.hpp"
#include "special_fields.hpp"
#include <cstddef>
#include <span>

namespace cpp_utils::serde
{

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, deserialize, );

namespace details
{

    template <typename input_t>
    std::span<const std::byte> to_byte_view(const input_t& input)
    {
        return std::as_bytes(std::span { input });
    }

    std::size_t _load_value_from_memory(std::span<const std::byte> input, std::size_t offset,
        types::concepts::fundamental_type auto& dest, const auto& parent_composite)
    {
        using T = std::decay_t<decltype(dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        dest = endianness::decode<endianness_t<parent_composite_t>, T>(
            reinterpret_cast<const char*>(input.data()) + offset);
        return offset + sizeof(T);
    }

    std::size_t _load_values_from_memory(std::span<const std::byte> input, std::size_t offset,
        types::concepts::fundamental_type auto* dest, std::size_t count,
        const auto& parent_composite)
    {
        using T = std::decay_t<decltype(*dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        endianness::decode_v<endianness_t<parent_composite_t>, T>(
            reinterpret_cast<const char*>(input.data()) + offset, count * sizeof(T), dest);
        return offset + sizeof(T) * count;
    }
}

std::size_t load_value(const auto& input, std::size_t offset,
    types::concepts::fundamental_type auto& dest, const auto& parent_composite)
{
    return details::_load_value_from_memory(
        details::to_byte_view(input), offset, dest, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, types::concepts::fundamental_type auto& field)
{
    (void)context;
    return load_value(parsing_context, offset, field, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, types::concepts::enum_type auto& field)
{
    using underlying_t = std::underlying_type_t<std::decay_t<decltype(field)>>;
    underlying_t& underlying_field = reinterpret_cast<underlying_t&>(field);
    return load_field(parent_composite, parsing_context, offset, context, underlying_field);
}

template <typename field_t>
concept dyn_array_field = dynamic_array_field<field_t> && !dynamic_array_until_eof_field<field_t>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dyn_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = details::resolve_field_size(parent_composite, array_field, context);
    if (count > 0)
    {
        array_field.resize(count);
        if constexpr (std::is_compound_v<field_t>)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                offset = deserialize(array_field[i],
                    std::forward<decltype(parsing_context)>(parsing_context), offset, context);
            }
            return offset;
        }
        else
        {
            return details::_load_values_from_memory(details::to_byte_view(parsing_context),
                offset, array_field.data(), count, parent_composite);
        }
    }
    return offset;
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dynamic_array_bytes_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto bytes = details::resolve_field_size(parent_composite, array_field, context);
    const auto count = bytes / sizeof(field_t);
    if (count > 0)
    {
        array_field.resize(count);
        details::_load_values_from_memory(details::to_byte_view(parsing_context), offset,
            array_field.data(), count, parent_composite);
    }
    return offset + bytes;
}

template <typename field_t>
concept dy_arr_until_eof_of_const_size = dynamic_array_until_eof_field<std::decay_t<field_t>>
    && const_size_field<typename std::decay_t<field_t>::value_type>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, dy_arr_until_eof_of_const_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    constexpr auto field_size = reflexion::field_size<field_t>();
    const auto count = (parsing_context.size() - offset) / field_size;

    if (count > 0)
    {
        array_field.resize(count);
        if constexpr (reflexion::can_split_v<field_t>)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                offset = deserialize(array_field[i],
                    std::forward<decltype(parsing_context)>(parsing_context), offset, context);
            }
            return offset;
        }
        else
        {
            return details::_load_values_from_memory(details::to_byte_view(parsing_context),
                offset, array_field.data(), count, parent_composite);
        }
    }
    return offset;
}

template <typename field_t>
concept dy_arr_until_eof_of_dyn_size
    = dynamic_array_until_eof_field<field_t> && !const_size_field<typename field_t::value_type>;


constexpr inline std::size_t load_field(const auto&, auto& parsing_context, std::size_t offset,
    const auto& context, dy_arr_until_eof_of_dyn_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;

    static_assert(std::is_compound_v<field_t>, "Only compound types are supported");
    while (offset < std::size(parsing_context))
    {
        array_field.emplace_back(field_t {});
        offset = deserialize(
            array_field.back(), std::forward<decltype(parsing_context)>(parsing_context), offset,
            context);
    }
    return offset;
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, static_array_field auto& array_field)
{
    (void)context;
    constexpr auto count = std::decay_t<decltype(array_field)>::array_size;
    return details::_load_values_from_memory(details::to_byte_view(parsing_context), offset,
        array_field.data(), count, parent_composite);
}

template <std::size_t size, uint8_t value>
constexpr inline std::size_t load_field(const auto&, auto&, std::size_t offset, const auto&,
    padding_bytes_t<size, value>&)
{
    return offset + size;
}

constexpr inline std::size_t load_field(const auto&, auto& parsing_context, std::size_t offset,
    const auto&, bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    const auto bytes = details::to_byte_view(parsing_context);
    std::size_t len = 0;
    while (len < field_t::max_len
        && std::to_integer<unsigned char>(bytes[offset + len]) != 0)
        ++len;
    field.value = std::string {
        reinterpret_cast<const char*>(bytes.data()) + offset, len
    };
    return offset + field_t::max_len;
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, const auto& context, unused_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    typename field_t::value_type discarded {};
    return load_field(parent_composite, parsing_context, offset, context, discarded);
}

template <typename composite_t, typename parsing_context_t, typename context_t>
constexpr inline std::size_t load_fields(
    const composite_t&, parsing_context_t&, std::size_t offset, const context_t&)
{
    return offset;
}

template <typename composite_t, typename parsing_context_t, typename context_t, typename T>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return deserialize(field, parsing_context, offset, context);
    else
        return load_field(r, parsing_context, offset, context, std::forward<T>(field));
}

template <typename composite_t, typename parsing_context_t, typename context_t, typename T,
    typename... Ts>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field, Ts&&... fields)
{
    offset = load_fields(r, parsing_context, offset, context, std::forward<T>(field));
    return load_fields(r, parsing_context, offset, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, deserialize, load_fields, );

template <typename composite_t, typename context_t = no_context>
constexpr composite_t deserialize(auto&& parsing_context, [[maybe_unused]] std::size_t offset = 0,
    const context_t& context = context_t {})
{
    composite_t r;
    deserialize(r, std::forward<decltype(parsing_context)>(parsing_context), offset, context);
    return r;
}

}
