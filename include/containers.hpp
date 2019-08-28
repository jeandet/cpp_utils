#ifndef CONTAINERS_H_INCLUDED
#define CONTAINERS_H_INCLUDED

#include "cpp_utils.hpp"
#include "types/detectors.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace cpp_utils::containers
{  
#define __iterator_type decltype(std::begin(std::declval<T>()))
    
#define __item_type decltype(*std::declval<__iterator_type>())
    
  HAS_METHOD(has_clear_method, clear )
  HAS_METHOD(has_erase_method, erase, __item_type)
  HAS_METHOD(has_front_method, front)
  HAS_METHOD(has_begin_method, begin)
  HAS_METHOD(has_end_method, end)
  HAS_METHOD(has_swap_method, swap, T)
  HAS_METHOD(has_size_method, size)
  HAS_METHOD(has_empty_method, empty)
  
/**  
  HAS_METHOD(back)
  HAS_METHOD(assign)
  HAS_METHOD(emplace)
  HAS_METHOD(insert)
  HAS_METHOD(emplace_front)
  HAS_METHOD(emplace_back)
  HAS_METHOD(push_front)
  HAS_METHOD(push_back)
  HAS_METHOD(pop_front)
  HAS_METHOD(pop_back)
  HAS_METHOD(at)
  **/
    
template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.front(), std::end(container), true)
{
    return std::find(std::cbegin(container), std::cend(container), value) != std::cend(container);
}

template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.find(value), std::cend(container), true)
{
    return container.find(value) != std::cend(container);
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value)
    -> decltype(0)
{
    static_assert(has_front_method_v<T1>,"Given container is missing front");
    return std::distance(
        std::cbegin(container), std::find(std::cbegin(container), std::cend(container), value));
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value)
    -> decltype(container.front().get(), std::is_pointer<T2>::value, 0)
{
    return std::distance(std::cbegin(container),
        std::find_if(std::cbegin(container), std::cend(container),
            [value](const auto& item) { return value == item.get(); }));
}

} //

#endif // CONTAINERS_H_INCLUDED
