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
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace cpp_utils::serde
{


template <std::size_t size, uint8_t value>
struct padding_bytes_t
{
    using do_not_split = reflexion::do_not_split_t;
    static constexpr std::size_t padding_size = size;
    static constexpr std::size_t wire_size = size;
    static constexpr uint8_t padding_value = value;

private:
    // Keeps padding_bytes_t a non-aggregate (single opaque slot for
    // reflexion::count_members, same trick static_array/dynamic_array rely on) instead of an
    // empty aggregate, which count_members's member-arity probe cannot handle when
    // padding_bytes_t isn't the composite's last member.
    char _reserved = 0;
};

template <typename field_t, std::size_t sz>
struct static_array
{
private:
    field_t _data[sz];

public:
    using do_not_split = reflexion::do_not_split_t;
    static constexpr std::size_t array_size = sz;
    using value_type = field_t;
    const value_type& operator[](std::size_t index) const { return _data[index]; }
    value_type& operator[](std::size_t index) { return _data[index]; }
    value_type* data() { return _data; }
    const value_type* data() const { return _data; }
    inline constexpr std::size_t size() const { return sz; }
};

template <typename T>
concept static_array_field = std::is_same_v<T, static_array<typename T::value_type, T::array_size>>;

template <std::size_t N>
struct bounded_string
{
    using do_not_split = reflexion::do_not_split_t;
    static constexpr std::size_t wire_size = N;
    static constexpr std::size_t max_len = N;
    std::string value;
};

template <typename T>
concept bounded_string_field = std::is_same_v<T, bounded_string<T::max_len>>;

template <std::size_t ID, typename field_t>
struct dynamic_array
{
private:
    std::vector<field_t> _data;

public:
    using do_not_split = reflexion::do_not_split_t;
    using dyn_size_field_tag = reflexion::dyn_size_field_tag_t;
    using value_type = field_t;
    static constexpr std::size_t id = ID;
    const value_type& operator[](std::size_t index) const { return _data[index]; }
    value_type& operator[](std::size_t index) { return _data[index]; }
    value_type* data() { return _data.data(); }
    const value_type* data() const { return _data.data(); }
    std::size_t size() const { return _data.size(); }
    void resize(std::size_t size) { _data.resize(size); }
    void push_back(const value_type& value) { _data.push_back(value); }
    void push_back(value_type&& value) { _data.push_back(std::move(value)); }
    void emplace_back(const value_type& value) { _data.emplace_back(value); }
    void emplace_back(value_type&& value) { _data.emplace_back(std::move(value)); }
    template <typename... Args>
    void emplace_back(Args&&... args) { _data.emplace_back(std::forward<Args>(args)...); }
    auto& back() { return _data.back(); }
    auto& front() { return _data.front(); }
    void clear() { _data.clear(); }
    auto begin() { return _data.begin(); }
    auto end() { return _data.end(); }
    auto begin() const { return _data.begin(); }
    auto end() const { return _data.end(); }
    auto cbegin() const { return _data.cbegin(); }
    auto cend() const { return _data.cend(); }
    auto rbegin() { return _data.rbegin(); }
    auto rend() { return _data.rend(); }
    auto rbegin() const { return _data.rbegin(); }
    auto rend() const { return _data.rend(); }
    auto crbegin() const { return _data.crbegin(); }
    auto crend() const { return _data.crend(); }
};

template <typename T>
concept dynamic_array_field = std::is_same_v<T, dynamic_array<T::id, typename T::value_type>>;

template <std::size_t ID, typename field_t>
struct dynamic_array_bytes : dynamic_array<ID, field_t>
{
};

template <typename T>
concept dynamic_array_bytes_field
    = std::is_same_v<T, dynamic_array_bytes<T::id, typename T::value_type>>;

template <typename T>
using dynamic_array_until_eof = dynamic_array<std::numeric_limits<std::size_t>::max(), T>;

template <typename field_t>
concept dynamic_array_until_eof_field
    = std::is_same_v<field_t, dynamic_array_until_eof<typename field_t::value_type>>;

template <typename field_t>
consteval bool _has_const_size()
{
    if constexpr (std::is_compound_v<field_t> && reflexion::can_split_v<field_t>)
    {
        return reflexion::composite_have_const_size<field_t>();
    }
    else
    {
        return !reflexion::is_dyn_size_field_v<std::decay_t<field_t>>;
    }
}

template <typename field_t>
concept const_size_field = _has_const_size<std::decay_t<field_t>>();


template <typename field_t>
concept dynamic_array_of_constant_size_field
    = dynamic_array_field<field_t> && const_size_field<typename field_t::value_type>;

namespace details
{
    struct dyn_size_base
    {
        using dyn_size_field_tag = reflexion::dyn_size_field_tag_t;
    };
}

// Primary template: T is constant-size (the common case, e.g. CDFpp's int32_t reserved
// fields). No base class here — unused<T> stays a plain single-member aggregate so
// `unused<int32_t>{-1}` aggregate-initializes `value` directly. (A base class subobject,
// even an empty one, would occupy the first slot in aggregate initialization and break
// that single-scalar brace-init syntax.)
template <typename T>
struct unused
{
    using do_not_split = reflexion::do_not_split_t;
    using value_type = T;

    T value {};
};

// T is dynamic-size: inherit dyn_size_base so is_dyn_size_field_v<unused<T>> propagates.
// save_field/field_runtime_size already reject this case via static_assert, so `value` is
// never written from here and the extra base subobject never needs scalar brace-init.
template <typename T>
    requires(!const_size_field<T>)
struct unused<T> : details::dyn_size_base
{
    using do_not_split = reflexion::do_not_split_t;
    using value_type = T;

    T value {};
};

template <typename T>
concept unused_field = std::is_same_v<T, unused<typename T::value_type>>;

template <typename wire_t, typename real_t, std::intmax_t Num, std::intmax_t Den = 1>
struct scaled
{
    using do_not_split = reflexion::do_not_split_t;
    using wire_type = wire_t;
    using value_type = real_t;
    static constexpr std::size_t wire_size = sizeof(wire_t);
    static constexpr std::intmax_t num = Num;
    static constexpr std::intmax_t den = Den;
    static constexpr real_t scale = static_cast<real_t>(Num) / static_cast<real_t>(Den);

    real_t value {};
};

template <typename T>
concept scaled_field
    = std::is_same_v<T, scaled<typename T::wire_type, typename T::value_type, T::num, T::den>>;

template <typename composite_t>
consteval auto _endianness()
{
    if constexpr (requires { typename composite_t::endianness; })
    {
        return typename composite_t::endianness {};
    }
    else
    {
        return endianness::little_endian_t {};
    }
}

template <typename composite_t>
using endianness_t = decltype(_endianness<composite_t>());

template <typename composite_t>
inline constexpr bool is_big_endian_v
    = std::is_same_v<endianness_t<composite_t>, endianness::big_endian_t>;


}

namespace std
{

std::size_t size(const cpp_utils::serde::dynamic_array_field auto& arr)
{
    return arr.size();
}

std::size_t size(const cpp_utils::serde::static_array_field auto& arr)
{
    return arr.size();
}


}
