#ifndef CONTAINERS_H
#define CONTAINERS_H
#include "cpp_utils.h"

#include <algorithm>
#include <map>
#include <vector>

namespace SciQLop::containers
{
  template<class T1, class T2>
  auto contains(const T1& container, const T2& value)
      -> decltype(container.front(), std::end(container), true)
  {
    return std::find(std::cbegin(container), std::cend(container), value) !=
           std::cend(container);
  }

  template<class T1, class T2>
  auto contains(const T1& container, const T2& value)
      -> decltype(container.find(value), std::cend(container), true)
  {
    return container.find(value) != std::cend(container);
  }

  template<class T1, class T2>
  auto index_of(const T1& container, const T2& value)
      -> decltype(container.front() == value, 0)
  {
    return std::distance(
        std::cbegin(container),
        std::find(std::cbegin(container), std::cend(container), value));
  }

  template<class T1, class T2>
  auto index_of(const T1& container, const T2& value)
      -> decltype(container.front().get(), std::is_pointer<T2>::value, 0)
  {
    return std::distance(std::cbegin(container),
                         std::find_if(std::cbegin(container),
                                      std::cend(container),
                                      [value](const auto& item) {
                                        return value == item.get();
                                      }));
  }

} // namespace SciQLop::containers

#endif // CONTAINERS_H
