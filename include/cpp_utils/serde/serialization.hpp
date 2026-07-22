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
#include "../io/sequential_writer.hpp"
#include "../reflexion/reflection.hpp"
#include "../types/concepts.hpp"
#include "context.hpp"
#include "special_fields.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace cpp_utils::serde
{

template <typename T>
concept byte_sink = requires(T t, std::size_t n) {
    { t.resize(n) };
    { t.data() };
    { t.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept serde_sink = byte_sink<T> || io::sequential_writer<T>;

SPLIT_FIELDS_FW_DECL(constexpr std::size_t, serialize_fields, const);

namespace details
{
    void ensure_size(byte_sink auto& sink, std::size_t needed)
    {
        if (sink.size() < needed)
            sink.resize(needed);
    }

    void _save_value_to_memory(serde_sink auto& sink, std::size_t offset,
        types::concepts::fundamental_type auto value, const auto& parent_composite)
    {
        using T = std::decay_t<decltype(value)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        char buffer[sizeof(T)];
        endianness::encode<endianness_t<parent_composite_t>>(value, buffer);
        if constexpr (io::sequential_writer<std::decay_t<decltype(sink)>>)
        {
            sink.write(buffer, sizeof(T));
        }
        else
        {
            ensure_size(sink, offset + sizeof(T));
            std::memcpy(reinterpret_cast<char*>(sink.data()) + offset, buffer, sizeof(T));
        }
    }

    void _save_values_to_memory(serde_sink auto& sink, std::size_t offset,
        const types::concepts::fundamental_type auto* values, std::size_t count,
        const auto& parent_composite)
    {
        using T = std::decay_t<decltype(*values)>;
        using parent_composite_t = std::decay_t<decltype(parent_composite)>;
        if constexpr (io::sequential_writer<std::decay_t<decltype(sink)>>)
        {
            std::vector<char> buffer(sizeof(T) * count);
            endianness::encode_v<endianness_t<parent_composite_t>>(values, count, buffer.data());
            sink.write(buffer.data(), buffer.size());
        }
        else
        {
            ensure_size(sink, offset + sizeof(T) * count);
            endianness::encode_v<endianness_t<parent_composite_t>>(
                values, count, reinterpret_cast<char*>(sink.data()) + offset);
        }
    }
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, types::concepts::fundamental_type auto field)
{
    (void)context;
    details::_save_value_to_memory(sink, offset, field, parent_composite);
    return offset + sizeof(field);
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, types::concepts::enum_type auto field)
{
    using underlying_t = std::underlying_type_t<std::decay_t<decltype(field)>>;
    return save_field(
        parent_composite, sink, offset, context, static_cast<underlying_t>(field));
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, const dynamic_array_field auto& array_field)
{
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = array_field.size();
    if (count == 0)
        return offset;
    if constexpr (std::is_compound_v<field_t>)
    {
        for (std::size_t i = 0; i < count; ++i)
            offset = serialize_fields(array_field[i], sink, offset, context);
        return offset;
    }
    else
    {
        details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
        return offset + sizeof(field_t) * count;
    }
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, const dynamic_array_bytes_field auto& array_field)
{
    (void)context;
    using array_field_t = std::decay_t<decltype(array_field)>;
    using field_t = typename array_field_t::value_type;
    const auto count = array_field.size();
    if (count > 0)
        details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
    return offset + sizeof(field_t) * count;
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, const static_array_field auto& array_field)
{
    (void)context;
    using array_field_t = std::decay_t<decltype(array_field)>;
    constexpr auto count = array_field_t::array_size;
    using field_t = typename array_field_t::value_type;
    details::_save_values_to_memory(sink, offset, array_field.data(), count, parent_composite);
    return offset + sizeof(field_t) * count;
}

template <std::size_t size, uint8_t value>
constexpr inline std::size_t save_field(const auto&, serde_sink auto& sink, std::size_t offset,
    const auto&, const padding_bytes_t<size, value>&)
{
    if constexpr (io::sequential_writer<std::decay_t<decltype(sink)>>)
    {
        sink.fill(static_cast<char>(value), size);
    }
    else
    {
        details::ensure_size(sink, offset + size);
        std::memset(reinterpret_cast<char*>(sink.data()) + offset, value, size);
    }
    return offset + size;
}

constexpr inline std::size_t save_field(const auto&, serde_sink auto& sink, std::size_t offset,
    const auto&, const bounded_string_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    char buffer[field_t::max_len] {};
    const auto copy_len = std::min(field.value.size(), field_t::max_len);
    std::memcpy(buffer, field.value.data(), copy_len);
    if constexpr (io::sequential_writer<std::decay_t<decltype(sink)>>)
    {
        sink.write(buffer, field_t::max_len);
    }
    else
    {
        details::ensure_size(sink, offset + field_t::max_len);
        std::memcpy(reinterpret_cast<char*>(sink.data()) + offset, buffer, field_t::max_len);
    }
    return offset + field_t::max_len;
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, const scaled_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    using wire_t = typename field_t::wire_type;
    const auto raw = static_cast<wire_t>(
        std::llround(static_cast<double>(field.value) / static_cast<double>(field_t::scale)));
    return save_field(parent_composite, sink, offset, context, raw);
}

constexpr inline std::size_t save_field(const auto& parent_composite, serde_sink auto& sink,
    std::size_t offset, const auto& context, const unused_field auto& field)
{
    using field_t = std::decay_t<decltype(field)>;
    using value_type = typename field_t::value_type;
    static_assert(const_size_field<value_type>,
        "cpp_utils::serde::unused<T>: T must be a constant-size field; dynamic-size T is not "
        "yet supported by save_field");
    return save_field(parent_composite, sink, offset, context, field.value);
}

template <typename composite_t, typename sink_t, typename context_t>
constexpr inline std::size_t save_fields(
    const composite_t&, sink_t&, std::size_t offset, const context_t&)
{
    return offset;
}

template <typename composite_t, typename sink_t, typename context_t, typename T>
constexpr inline std::size_t save_fields(const composite_t& r, sink_t& sink,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field)
{
    using Field_t = std::decay_t<T>;
    constexpr std::size_t count = reflexion::count_members<Field_t>;
    if constexpr (reflexion::can_split_v<Field_t> && (count >= 1))
        return serialize_fields(field, sink, offset, context);
    else
        return save_field(r, sink, offset, context, std::forward<T>(field));
}

template <typename composite_t, typename sink_t, typename context_t, typename T, typename... Ts>
constexpr inline std::size_t save_fields(const composite_t& r, sink_t& sink,
    [[maybe_unused]] std::size_t offset, const context_t& context, T&& field, Ts&&... fields)
{
    offset = save_fields(r, sink, offset, context, std::forward<T>(field));
    return save_fields(r, sink, offset, context, std::forward<Ts>(fields)...);
}

SPLIT_FIELDS(constexpr std::size_t, serialize_fields, save_fields, const);

// For a byte_sink, the return value is the offset reached within `sink` relative to
// `offset` — meaningful as an absolute address only when the caller treats `sink` as
// a single buffer addressed from 0 (multiple serialize() calls at different `offset`
// values into the same buffer is a valid, supported pattern). A sequential_writer has
// no such addressable buffer — it only ever appends at its own internal cursor — so
// the position that actually matters after the call is the writer's own sink.offset(),
// not the call-local `offset + bytes_written_this_call` value serialize_fields
// computes (which resets to `offset` on every call and knows nothing of previous
// writes through the same writer). Without this, chaining multiple serialize() calls
// through the same sequential_writer (as any record-oriented format's save path
// does) silently returns a value `bytes_written_this_call` short of the writer's true
// position on every call after the first.
template <typename composite_t, serde_sink sink_t, typename context_t = no_context>
constexpr std::size_t serialize(const composite_t& value, sink_t& sink,
    std::size_t offset = 0, const context_t& context = context_t {})
{
    const auto result = serialize_fields(value, sink, offset, context);
    if constexpr (io::sequential_writer<sink_t>)
        return sink.offset();
    else
        return result;
}

}
