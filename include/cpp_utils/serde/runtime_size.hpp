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
#include "../reflexion/reflection.hpp"
#include "context.hpp"
#include "special_fields.hpp"

namespace cpp_utils::serde
{

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, runtime_size_fields, const);

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const types::concepts::fundamental_type auto& field)
{
    return sizeof(field);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const types::concepts::enum_type auto& field)
{
    return sizeof(field);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const dynamic_array_field auto& array_field)
{
    using field_t = typename std::decay_t<decltype(array_field)>::value_type;
    return array_field.size() * reflexion::field_size<field_t>();
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const dynamic_array_bytes_field auto& array_field)
{
    using field_t = typename std::decay_t<decltype(array_field)>::value_type;
    return array_field.size() * sizeof(field_t);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const static_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    return array_field_t::array_size * sizeof(field_t);
}

constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    return field_t::max_len;
}

template <std::size_t size, uint8_t value>
constexpr inline std::size_t field_runtime_size(
    const auto&, const auto&, const padding_bytes_t<size, value>&)
{
    return size;
}

constexpr inline std::size_t field_runtime_size(
    const auto& parent, const auto& context, const unused_field auto& field)
{
    using value_type = typename std::decay_t<decltype(field)>::value_type;
    static_assert(const_size_field<value_type>,
        "cpp_utils::serde::unused<T>: T must be a constant-size field; dynamic-size T is not "
        "yet supported by runtime_size (save_field does not support it either)");
    value_type zero {};
    return field_runtime_size(parent, context, zero);
}

template <typename composite_t, typename context_t, typename T>
constexpr inline std::size_t fields_runtime_size(
    const composite_t& r, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return runtime_size_fields(field, context);
    else
        return field_runtime_size(r, context, std::forward<T>(field));
}

template <typename composite_t, typename context_t, typename T, typename... Ts>
constexpr inline std::size_t fields_runtime_size(
    const composite_t& r, const context_t& context, T&& field, Ts&&... fields)
{
    return fields_runtime_size(r, context, std::forward<T>(field))
        + fields_runtime_size(r, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, runtime_size_fields, fields_runtime_size, const);

template <typename composite_t, typename context_t = no_context>
constexpr std::size_t runtime_size(
    const composite_t& value, const context_t& context = context_t {})
{
    return runtime_size_fields(value, context);
}

}
