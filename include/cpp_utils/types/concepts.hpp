#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2024, Plasma Physics Laboratory - CNRS
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
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>

namespace cpp_utils::types::concepts
{

/** pointer_to_contiguous_memory concept, requires pointer arithmetic and dereference */
template <class T>
concept pointer_to_contiguous_memory = requires(T t) {
    { *t } -> std::convertible_to<std::remove_pointer_t<T>>;
    { t + 1 } -> std::same_as<T>;
    { t - 1 } -> std::same_as<T>;
    { t += 1 } -> std::same_as<T&>;
    { t -= 1 } -> std::same_as<T&>;
    { t[0] } -> std::convertible_to<std::remove_pointer_t<T>>;
};

template <class T>
concept fundamental_type = std::is_fundamental_v<std::decay_t<T>>;

template <class T>
concept enum_type = std::is_enum_v<std::decay_t<T>>;

template <class T>
concept container = requires(T t) {
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.max_size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
    { t.cbegin() } -> std::input_or_output_iterator;
    { t.cend() } -> std::input_or_output_iterator;
};

template <class T>
concept sequence_container = container<T> && requires(T t) {
    { t.front() } -> std::convertible_to<typename std::decay_t<T>::value_type>;
};

template <class T>
concept contiguous_sequence_container = container<T> &&
        std::contiguous_iterator<typename std::decay_t<T>::iterator>;

template <class T>
concept bounded_array = std::is_bounded_array_v<T>;

/** random_access_buffer concept: a read-only byte buffer that can copy an arbitrary
 * (offset, size) range into a caller-owned destination (read), hand back a zero-copy
 * pointer at an arbitrary offset (view), report its size, and report whether it was
 * successfully opened/constructed (is_valid). Satisfied by cpp_utils::io::memory_mapped_file
 * and cpp_utils::io::buffer_view. */
template <class T>
concept random_access_buffer = requires(const T& t, char* dest, std::size_t offset,
    std::size_t size) {
    { t.read(dest, offset, size) };
    { t.view(offset) };
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.is_valid() } -> std::convertible_to<bool>;
};

/** contiguous_sized_range: a range whose elements are contiguous in memory and whose size
 * is known in O(1) — std::vector, std::array, std::span, no_init_vector, etc. Shared
 * precondition for all threading::parallel_chunks_* input ranges. */
template <class T>
concept contiguous_sized_range = std::ranges::contiguous_range<T> && std::ranges::sized_range<T>;

/** chunk_for_each_callback: F is invocable with a mutable span over Input's element type —
 * the per-chunk callback shape used by threading::parallel_chunks_for_each. */
template <class F, class Input>
concept chunk_for_each_callback = contiguous_sized_range<Input> &&
    std::invocable<F, std::span<std::ranges::range_value_t<Input>>>;

} // namespace cpp_utils::types::concepts

