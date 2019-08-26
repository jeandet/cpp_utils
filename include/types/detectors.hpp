#pragma once
#include <type_traits>

#define HAS_METHOD(method)                                                                         \
    template <class T, class = void>                                                               \
    struct has_##method##_method : std::false_type                                                 \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    struct has_##method##_method<T,                                                                \
        std::void_t<std::is_member_function_pointer<decltype(&T::method)>>>                        \
            : std::is_member_function_pointer<decltype(&T::method)>                                \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##method##_method_v = has_##method##_method<T>::value;

#define HAS_MEMBER(member)                                                                         \
    template <class T, class = void>                                                               \
    struct has_##member##_member_object : std::false_type                                          \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    struct has_##member##_member_object<T, decltype(std::declval<T>().member, void())>             \
            : std::is_member_object_pointer<decltype(&T::member)>                                  \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##member##_member_object_v                                    \
        = has_##member##_member_object<T>::value;

namespace cpp_utils::types::detectors
{


template <typename T, typename = void>
struct is_qt_tree_item : std::false_type
{
};

template <typename T>
struct is_qt_tree_item<T,
    decltype(std::declval<T>().takeChildren(), std::declval<T>().parent(),
        std::declval<T>().addChild(nullptr))> : std::true_type
{
};

template <class T>
static inline constexpr bool is_qt_tree_item_v = is_qt_tree_item<T>::value;


HAS_METHOD(toStdString)
}
