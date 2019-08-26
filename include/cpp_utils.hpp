#ifndef CPP_UTILS_H
#define CPP_UTILS_H

#include "types/detectors.hpp"
#include "types/pointers.hpp"

#include <functional>
#include <tuple>
#include <type_traits>

// taken from here https://www.fluentcpp.com/2017/10/27/function-aliases-cpp/
#define ALIAS_TEMPLATE_FUNCTION(highLevelF, lowLevelF)                         \
  template<typename... Args>                                                   \
  inline auto highLevelF(Args&&... args)                                       \
      ->decltype(lowLevelF(std::forward<Args>(args)...))                       \
  {                                                                            \
    return lowLevelF(std::forward<Args>(args)...);                             \
  }


template < typename T > constexpr T diff (const std::pair < T, T > &p)
{
  return p.second - p.first;
}

template < typename T > constexpr auto const &
to_value (const T & item)
{
  if constexpr
    (std::is_pointer_v < std::remove_reference_t < std::remove_cv_t < T >>>)
    {
      return *item;
    }
  else
    {
      if constexpr
	(cpp_utils::types::pointers::is_smart_ptr_v < T >)
	{
	  return *item.get ();
	}
      else
	{
	  return item;
	}
    }
}

template < typename T > auto repeat_n (T func, int number)->
decltype (func (), void ())
{
  for (int i = 0; i < number; i++)
    func ();
}

template < typename T > auto repeat_n (T func, int number)->
decltype (func (1), void ())
{
  for (int i = 0; i < number; i++)
    func (i);
}



inline int
operator* (int number, const std::function < void (void) > &func)
{
  for (int i = 0; i < number; i++)
    func ();
  return number;
}

template < typename T > std::string to_std_string (const T & text)
{
  if constexpr
    (std::is_same_v < std::string, T >)
    {
      return text;
    }
  else if constexpr
    (cpp_utils::types::detectors::has_toStdString_method_v < T >)
    {
      return text.toStdString ();
    }
}

#endif // CPP_UTILS_H
