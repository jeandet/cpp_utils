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
#include <vector>

namespace cpp_utils::serde
{


template <std::size_t size, uint8_t value>
struct padding_bytes_t
{
    using field_tag = reflexion::field_tag_t;
    static constexpr std::size_t padding_size = size;
    static constexpr uint8_t padding_value = value;
};

template <typename field_t, std::size_t size>
struct static_array
{
private:
    field_t _data[size];
public:
using field_tag = reflexion::field_tag_t;
    static constexpr std::size_t array_size = size;
    using value_type = field_t;
    const value_type& operator[](std::size_t index) const { return _data[index]; }
    value_type& operator[](std::size_t index) { return _data[index]; }
    value_type* data() { return _data; }
    const value_type* data() const { return _data; }
};

template <typename T>
concept static_array_field = std::is_same_v<T, static_array<typename T::value_type, T::array_size>>;

template <std::size_t ID, typename field_t>
struct dynamic_array
{
    private:
    std::vector<field_t> _data;
    public:
    using field_tag = reflexion::field_tag_t;
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
