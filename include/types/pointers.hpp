#ifndef POINTERS_HPP_INCLUDED
#define POINTERS_HPP_INCLUDED

#include <memory>
#include <type_traits>

namespace cpp_utils::types::pointers
{

template <typename T, typename = void>
struct is_dereferencable : std::false_type
{
};

template <typename T>
struct is_dereferencable<T, decltype(*std::declval<T>(), void())> : std::true_type
{
};

template <class T>
static inline constexpr bool is_dereferencable_v = is_dereferencable<T>::value;

template <typename T, typename = void>
struct is_std_shared_ptr : std::false_type
{
};

template <typename T>
struct is_std_shared_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::shared_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_shared_ptr_v = is_std_shared_ptr<T>::value;

template <typename T, typename = void>
struct is_std_unique_ptr : std::false_type
{
};

template <typename T>
struct is_std_unique_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::unique_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_unique_ptr_v = is_std_unique_ptr<T>::value;

template <typename T, typename = void>
struct is_std_weak_ptr : std::false_type
{
};

template <typename T>
struct is_std_weak_ptr<T, decltype(std::declval<typename T::element_type>(), void())>
        : std::is_same<T, std::weak_ptr<typename T::element_type>>
{
};

template <class T>
static inline constexpr bool is_std_weak_ptr_v = is_std_weak_ptr<T>::value;


template <typename T>
struct is_std_smart_ptr: std::disjunction<is_std_shared_ptr<T>, is_std_unique_ptr<T>, is_std_weak_ptr<T>>
{
};

template <class T>
static inline constexpr bool is_std_smart_ptr_v = is_std_smart_ptr<T>::value;

template <typename T, typename = void>
struct is_smart_ptr: std::false_type
{
};

template <typename T>
struct is_smart_ptr<T,std::enable_if_t<std::disjunction_v<is_std_smart_ptr<T>>>>: std::true_type
{
};



template <class T>
static inline constexpr bool is_smart_ptr_v = is_smart_ptr<T>::value;
}

#endif // POINTERS_HPP_INCLUDED
