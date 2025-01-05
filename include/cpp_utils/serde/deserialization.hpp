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
            input_address + offset, count * sizeof(T), dest);
        return offset + sizeof(T) * count;
    }

    const char* _pointer_to_memory(const auto& input)
    {
        using input_t = std::decay_t<decltype(input)>;
        if constexpr (requires { std::data(input); })
        {
            return reinterpret_cast<const char*>(std::data(input));
        }
        if constexpr (requires { input.data(); })
        {
            return reinterpret_cast<const char*>(input.data());
        }
        if constexpr (std::is_pointer_v<input_t>)
        {
            return reinterpret_cast<const char*>(input);
        }
        throw std::runtime_error("Unsupported input type");
    }
}

std::size_t load_value(const auto& input, std::size_t offset,
    types::concepts::fundamental_type auto& dest, const auto& parent_composite)
{
    return details::_load_value_from_memory(
        details::_pointer_to_memory(input), offset, dest, parent_composite);
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, types::concepts::fundamental_type auto& field)
{
    return load_value(parsing_context, offset, field, parent_composite);
}

template <typename field_t>
concept dyn_array_field = dynamic_array_field<field_t> && !dynamic_array_until_eof_field<field_t>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, dyn_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = parent_composite.field_size(array_field);
    array_field.resize(count);
    if constexpr (std::is_compound_v<field_t>)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            offset = deserialize(
                array_field[i], std::forward<decltype(parsing_context)>(parsing_context), offset);
        }
        return offset;
    }
    else
    {
        return details::_load_values_from_memory(details::_pointer_to_memory(parsing_context),
            offset, array_field.data(), count, parent_composite);
    }
}

template <typename field_t>
concept dy_arr_until_eof_of_const_size
    = dynamic_array_until_eof_field<std::decay_t<field_t>> && const_size_field<typename std::decay_t<field_t>::value_type>;

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, dy_arr_until_eof_of_const_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    constexpr auto field_size = reflexion::field_size<field_t>();
    const auto count = (parsing_context.size() - offset) / field_size;

    array_field.resize(count);
    if constexpr (std::is_compound_v<field_t>)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            offset = deserialize(
                array_field[i], std::forward<decltype(parsing_context)>(parsing_context), offset);
        }
        return offset;
    }
    else
    {
        return details::_load_values_from_memory(details::_pointer_to_memory(parsing_context),
            offset, array_field.data(), count, parent_composite);
    }
}

template <typename field_t>
concept dy_arr_until_eof_of_dyn_size
    = dynamic_array_until_eof_field<field_t> && !const_size_field<typename field_t::value_type>;


constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, dy_arr_until_eof_of_dyn_size auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;

    static_assert(std::is_compound_v<field_t>, "Only compound types are supported");
    while (offset < std::size(parsing_context))
    {
        array_field.emplace_back(field_t {});
        offset = deserialize(
            array_field.back(), std::forward<decltype(parsing_context)>(parsing_context), offset);
    }
    return offset;
}

constexpr inline std::size_t load_field(const auto& parent_composite, auto& parsing_context,
    std::size_t offset, static_array_field auto& array_field)
{
    constexpr auto count = std::decay_t<decltype(array_field)>::array_size;
    return details::_load_values_from_memory(details::_pointer_to_memory(parsing_context), offset,
        array_field.data(), count, parent_composite);
}

template <typename composite_t, typename parsing_context_t, typename T>
constexpr inline std::size_t load_fields(const composite_t& r, parsing_context_t& parsing_context,
    [[maybe_unused]] std::size_t offset, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (std::is_compound_v<Field_t> && (count >= 1)
        && (not reflexion::is_field_v<Field_t>))
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
