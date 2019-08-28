#pragma once

#include "../cpp_utils.hpp"
#include "../types/detectors.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace cpp_utils::containers
{

IS_TEMPLATE_T(is_std_list, std::list);
IS_TEMPLATE_T(is_std_forward_list, std::forward_list);
IS_TEMPLATE_T(is_std_vector, std::vector);
IS_TEMPLATE_T(is_std_deque, std::deque);
IS_TEMPLATE_T(is_std_basic_string, std::basic_string);

template <class T>
struct is_std_array : std::false_type
{
};
template <class T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type
{
};
template <typename T>
static inline constexpr bool is_std_array_v = is_std_array<T>::value;

#define __iterator_type decltype(std::begin(std::declval<T>()))

#define __item_type decltype(*std::declval<__iterator_type>())

HAS_METHOD(has_clear_method, clear)
HAS_METHOD(has_erase_method, erase, __item_type)
HAS_METHOD(has_front_method, front)
HAS_METHOD(has_begin_method, begin)
HAS_METHOD(has_end_method, end)
HAS_METHOD(has_cbegin_method, cbegin)
HAS_METHOD(has_cend_method, cend)
HAS_METHOD(has_swap_method, swap, T&)
HAS_METHOD(has_size_method, size)
HAS_METHOD(has_max_size_method, max_size)
HAS_METHOD(has_empty_method, empty)
HAS_METHOD(has_insert_method, insert, __iterator_type, __item_type)
HAS_METHOD(has_at_method, at, int)

HAS_TYPE(value_type)
HAS_TYPE(reference)
HAS_TYPE(const_reference)
HAS_TYPE(iterator)
HAS_TYPE(const_iterator)
HAS_TYPE(difference_type)
HAS_TYPE(size_type)

template <class T>
inline constexpr bool _is_container()
{
    constexpr bool _is_std_container = std::disjunction_v<is_std_list<T>, is_std_forward_list<T>>;
    static_assert(_is_std_container | has_value_type_type_v<T>, "");
    static_assert(_is_std_container | has_reference_type_v<T>, "");
    static_assert(_is_std_container | has_const_reference_type_v<T>, "");
    static_assert(_is_std_container | has_iterator_type_v<T>, "");
    static_assert(_is_std_container | has_const_iterator_type_v<T>, "");
    static_assert(_is_std_container | has_difference_type_type_v<T>, "");
    static_assert(_is_std_container | has_size_type_type_v<T>, "");
    static_assert(_is_std_container | has_begin_method_v<T>, "");
    static_assert(_is_std_container | has_end_method_v<T>, "");
    static_assert(_is_std_container | has_cbegin_method_v<T>, "");
    static_assert(_is_std_container | has_cend_method_v<T>, "");
    static_assert(_is_std_container | has_swap_method_v<T>, "");
    static_assert(_is_std_container | has_size_method_v<T>, "");
    static_assert(_is_std_container | has_max_size_method_v<T>, "");
    static_assert(_is_std_container | has_empty_method_v<T>, "");
    return true;
}

template <typename T>
struct is_container
        : std::disjunction<
              std::conjunction<has_value_type_type<T>, has_reference_type<T>,
                  has_const_reference_type<T>, has_iterator_type<T>, has_const_iterator_type<T>,
                  has_difference_type_type<T>, has_size_type_type<T>, has_begin_method<T>,
                  has_end_method<T>, has_cbegin_method<T>, has_cend_method<T>, has_swap_method<T>,
                  has_size_method<T>, has_max_size_method<T>, has_empty_method<T>>,
              is_std_list<T>, is_std_forward_list<T>>
{
};

template <class T>
static inline constexpr bool is_container_v = is_container<T>::value;


// SequenceContainer  TODO complete tis later
template <class T>
inline constexpr bool _is_sequence_container()
{
    static_assert(is_container_v<T>, "");
    return true;
}

template <typename T>
struct is_sequence_container
        : std::disjunction<std::conjunction<is_container<T>, has_insert_method<T>>, is_std_list<T>,
              is_std_forward_list<T>, is_std_array<T>>
{
};

template <class T>
static inline constexpr bool is_sequence_container_v = is_sequence_container<T>::value;

} //

