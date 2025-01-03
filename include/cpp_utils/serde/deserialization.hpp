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
#include "special_fields.hpp"

namespace cpp_utils::serde
{

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, deserialize, );

template <typename input_t, typename field_t, typename parent_composite_t>
std::size_t load_field(const input_t& input, std::size_t offset, field_t& field,
    const parent_composite_t& parent_composite);


namespace details
{

    std::size_t _load_value_from_memory(const char* const input_address, std::size_t offset,
        types::concepts::fundamental_type auto& dest, const auto& parent_composite)
    {
        using T = std::decay_t<decltype(dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        dest = endianness::decode<endianness_t<parent_composite_t>, T>(input_address + offset);
        return offset + sizeof(T);
    }

    std::size_t _load_values_from_memory(const char* const input_address, std::size_t offset,
        types::concepts::fundamental_type auto* dest, std::size_t count,
        const auto& parent_composite)
    {
        using T = std::decay_t<decltype(*dest)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        endianness::decode_v<endianness_t<parent_composite_t>, T>(
            input_address + offset, count*sizeof(T), dest);
        return offset + sizeof(T) * count;
    }
}

std::size_t load_value(const char* input, std::size_t offset,
    types::concepts::fundamental_type auto& dest, const auto& parent_composite)
{
    return details::_load_value_from_memory(input, offset, dest, parent_composite);
}

std::size_t load_value(const types::concepts::contiguous_sequence_container auto& input,
    std::size_t offset, types::concepts::fundamental_type auto& dest, const auto& parent_composite)
{
    return details::_load_value_from_memory(
        reinterpret_cast<const char*>(std::data(input)), offset, dest, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, types::concepts::fundamental_type auto& field)
{
    return load_value(parsing_context, offset, field, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, dynamic_array_field auto& array_field)
{
    using input_t = std::decay_t<decltype(parsing_context)>;
    using field_t = typename std::decay_t<decltype(array_field)>::value_type;
    const auto count = parent_composite.field_size(array_field);
    array_field.resize(count);
    if constexpr (std::is_compound_v<field_t>)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            offset = deserialize(array_field[i], std::forward<decltype(parsing_context)>(parsing_context), offset);
        }
        return offset;
    }
    else
    {
        if constexpr (types::concepts::contiguous_sequence_container<input_t>)
        {
            return details::_load_values_from_memory(
                reinterpret_cast<const char*>(std::data(parsing_context)), offset, array_field.data(), count,
                parent_composite);
        }
        if constexpr (std::is_pointer_v<input_t>)
        {
            return details::_load_values_from_memory(reinterpret_cast<const char*>(parsing_context),
                offset, array_field.data(), count, parent_composite);
        }
    }
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, static_array_field auto& array_field)
{
    using input_t = std::decay_t<decltype(parsing_context)>;
    constexpr auto count = std::decay_t<decltype(array_field)>::array_size;
    if constexpr (types::concepts::contiguous_sequence_container<input_t>)
    {
        return details::_load_values_from_memory(
            reinterpret_cast<const char*>(std::data(parsing_context)), offset, array_field.data(), count,
            parent_composite);
    }
    if constexpr (std::is_pointer_v<input_t>)
    {
        return details::_load_values_from_memory(reinterpret_cast<const char*>(parsing_context),
            offset, array_field.data(), count, parent_composite);
    }
}

template <typename composite_t, typename parsing_context_t, typename T>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (std::is_compound_v<Field_t> && (count >= 1) && (not reflexion::is_field_v<Field_t>))
        return deserialize(field, parsing_context, offset);
    else
        return load_field(r, parsing_context, offset, std::forward<T>(field));
}

template <typename composite_t, typename parsing_context_t, typename T, typename... Ts>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, T&& field, Ts&&... fields)
{
    offset = load_fields(r, parsing_context, offset, std::forward<T>(field));
    return load_fields(r, parsing_context, offset, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, deserialize, load_fields, );

template <typename composite_t>
constexpr composite_t deserialize(auto&& parsing_context, [[maybe_unused]] std::size_t offset = 0)
{
    composite_t r;
    deserialize(r, std::forward<decltype(parsing_context)>(parsing_context), offset);
    return r;
}

}
