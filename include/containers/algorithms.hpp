#pragma once

#include "../cpp_utils.hpp"
#include "../types/detectors.hpp"
#include "../containers/traits.hpp"
#include <algorithm>
#include <string>

namespace cpp_utils::containers
{
template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.front(), std::end(container), true)
{
    static_assert(is_container_v<T1>, "");
    return std::find(std::cbegin(container), std::cend(container), value) != std::cend(container);
}

template <class T1, class T2>
auto contains(const T1& container, const T2& value)
    -> decltype(container.find(value), std::cend(container), true)
{
    static_assert(is_container_v<T1>, "");
    return container.find(value) != std::cend(container);
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value) -> decltype(0)
{
    static_assert(is_sequence_container_v<T1>, "");
    return std::distance(
        std::cbegin(container), std::find(std::cbegin(container), std::cend(container), value));
}

template <class T1, class T2>
auto index_of(const T1& container, const T2& value)
    -> decltype(container.front().get(), std::is_pointer<T2>::value, 0)
{
    static_assert(is_container_v<T1>, "");
    return std::distance(std::cbegin(container),
        std::find_if(std::cbegin(container), std::cend(container),
            [value](const auto& item) { return value == item.get(); }));
}

template <class input_t, template <typename val_t, typename...> class output_t, typename value_t>
auto split(const input_t& input, output_t<input_t>& output, const value_t& splitVal) -> decltype(void())
{
    static_assert(is_container_v<input_t>, "");
    static_assert(is_container_v<output_t<input_t>>, "");
    if (std::size(input) == 0)
        return;
    auto current = std::cbegin(input);
    auto end = std::cend(input);
    input_t chunk;
    while (current != end)
    {
        if (*current != splitVal)
            chunk.push_back(*current);
        else
        {
            if (std::size(chunk))
            {
                output.push_back(std::move(chunk));
                chunk = input_t {};
            }
        }
        current++;
    }
    if (std::size(chunk))
        output.push_back(std::move(chunk));
}

} //
