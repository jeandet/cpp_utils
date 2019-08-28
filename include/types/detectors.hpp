#pragma once
#include <type_traits>


// https://stackoverflow.com/questions/28309164/checking-for-existence-of-an-overloaded-member-function
#define HAS_METHOD(name, method, ...)                                                              \
    template <typename T, typename... Args>                                                        \
    struct _##name                                                                                 \
    {                                                                                              \
        template <typename C,                                                                      \
            typename = decltype(std::declval<C>().method(std::declval<Args>()...))>                \
        static std::true_type test(int);                                                           \
        template <typename C>                                                                      \
        static std::false_type test(...);                                                          \
                                                                                                   \
    public:                                                                                        \
        static constexpr bool value = decltype(test<T>(0))::value;                                 \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    struct name : std::integral_constant<bool, _##name<T, ##__VA_ARGS__>::value>                   \
    {                                                                                              \
    };                                                                                             \
    template <typename T>                                                                          \
    static inline constexpr bool name##_v = _##name<T, ##__VA_ARGS__>::value;


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


#define HAS_TYPE(type)                                                                             \
    template <typename T, typename = void>                                                         \
    struct has_##type##_type : std::false_type                                                     \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <typename T>                                                                          \
    struct has_##type##_type<T, decltype(std::declval<typename T::type>(), void())>                \
            : std::true_type                                                                       \
    {                                                                                              \
    };                                                                                             \
                                                                                                   \
    template <class T>                                                                             \
    static inline constexpr bool has_##type##_type_v = has_##type##_type<T>::value;

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


HAS_METHOD(has_toStdString_method, toStdString)
}
