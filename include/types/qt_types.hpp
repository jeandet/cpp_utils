#ifndef QT_TYPES_HPP_INCLUDED
#define QT_TYPES_HPP_INCLUDED

#include "pointers.hpp"
#include <type_traits>

namespace cpp_utils::types::pointers
{

template <typename T, typename = void>
struct is_QExplicitlySharedDataPointer : std::false_type
{
};

template <typename T>
struct is_QExplicitlySharedDataPointer<T,
    decltype(std::declval<T>().take(), !std::declval<T>(), std::declval<T>().data(),
        std::declval<T>().reset(), std::declval<T>().detach(), void())>
        : std::conjunction<is_dereferencable<T>>
{
};

template <class T>
static inline constexpr bool is_QExplicitlySharedDataPointer_v
    = is_QExplicitlySharedDataPointer<T>::value;
    

template <typename T, typename = void>
struct is_QSharedDataPointer : std::false_type
{
};

template <typename T>
struct is_QSharedDataPointer<T,
    decltype(std::declval<T>().constData(), std::declval<T>().data(), std::declval<T>().detach(),
        void())> : std::conjunction<is_dereferencable<T>,std::negation<is_QExplicitlySharedDataPointer<T>>>
{
};

template <class T>
static inline constexpr bool is_QSharedDataPointer_v = is_QSharedDataPointer<T>::value;


template <typename T, typename = void>
struct is_QSharedPointer : std::false_type
{
};

template <typename T>
struct is_QSharedPointer<T,
    decltype(std::declval<T>().get(), std::declval<T>().isNull(), std::declval<T>().clear(),
        std::declval<T>().data(), std::declval<T>().reset(), void())>
        : std::conjunction<is_dereferencable<T>,std::negation<is_QExplicitlySharedDataPointer<T>>>
{
};

template <class T>
static inline constexpr bool is_QSharedPointer_v = is_QSharedPointer<T>::value;


template <typename T, typename = void>
struct is_QWeakPointer : std::false_type
{
};

template <typename T>
struct is_QWeakPointer<T,
    decltype(std::declval<T>().clear(), std::declval<T>().data(), std::declval<T>().isNull(),
        std::declval<T>().toStrongRef(), std::declval<T>().lock(), void())> : std::true_type
{
};

template <class T>
static inline constexpr bool is_QWeakPointer_v = is_QWeakPointer<T>::value;

template <typename T, typename = void>
struct is_QScopedArrayPointer : std::false_type
{
};

template <typename T>
struct is_QScopedArrayPointer<T,
    decltype(std::declval<T>()[0], std::declval<T>().take(), std::declval<T>().get(),
        std::declval<T>().data(), std::declval<T>().reset(nullptr), std::declval<T>().isNull(),
        void())> : is_dereferencable<T>
{
};

template <class T>
static inline constexpr bool is_QScopedArrayPointer_v = is_QScopedArrayPointer<T>::value;


template <typename T, typename = void>
struct is_QScopedPointer : std::false_type
{
};

template <typename T>
struct is_QScopedPointer<T,
    decltype(std::declval<T>().take(), std::declval<T>().get(), std::declval<T>().data(),
        std::declval<T>().reset(nullptr), std::declval<T>().isNull(), void())>
        : std::conjunction<is_dereferencable<T>, std::negation<is_QScopedArrayPointer<T>>>
{
};

template <class T>
static inline constexpr bool is_QScopedPointer_v = is_QScopedPointer<T>::value;


template <typename T>
struct is_qt_smart_ptr : std::disjunction<is_QSharedDataPointer<T>,
                             is_QExplicitlySharedDataPointer<T>, is_QSharedPointer<T>,
                             is_QWeakPointer<T>, is_QScopedPointer<T>, is_QScopedArrayPointer<T>>
{
};

template <class T>
static inline constexpr bool is_qt_smart_ptr_v = is_qt_smart_ptr<T>::value;

template <typename T>
struct is_smart_ptr<T, std::enable_if_t<std::disjunction_v<is_qt_smart_ptr<T>>>> : std::true_type
{
};


template <typename T, typename = void>
struct is_smart_ptr2 : std::false_type
{
};

template <typename T>
struct is_smart_ptr2<T, std::enable_if_t<std::disjunction_v<is_qt_smart_ptr<T>>>> : std::true_type
{
};

}

#endif // QT_TYPES_HPP_INCLUDED
