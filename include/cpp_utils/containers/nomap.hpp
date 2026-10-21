/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the “Software”), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#pragma once
#include <algorithm>
#include <compare>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace cpp_utils::containers
{
namespace details
{
template <typename T>
using base_type_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, bool is_const>
using ref_t = std::conditional_t<is_const, const base_type_t<T>&, base_type_t<T>&>;
}

template <typename Key, typename T>
struct nomap_node : public std::pair<Key, details::base_type_t<T>>
{
    using key_type = Key;
    using mapped_type = details::base_type_t<T>;


    nomap_node() : p_empty { true } { }
    nomap_node(Key&& k, mapped_type&& v)
            : std::pair<Key, mapped_type> { std::move(k), std::move(v) }
    {
    }
    nomap_node(const Key& k, const mapped_type& v) : std::pair<Key, mapped_type> { k, v } { }
    nomap_node(const Key& k, mapped_type&& v) : std::pair<Key, mapped_type> { k, std::move(v) } { }

    nomap_node(nomap_node&&) = default;
    nomap_node(const nomap_node&) = default;
    nomap_node& operator=(const nomap_node&) = default;
    nomap_node& operator=(nomap_node&&) = default;

    [[nodiscard]] inline auto& key() const noexcept { return this->first; }
    [[nodiscard]] inline auto& mapped() const noexcept { return this->second; }
    [[nodiscard]] inline bool empty() const noexcept { return this->p_empty; };

    friend void swap(nomap_node& lhs, nomap_node& rhs)
    {
        std::swap(lhs.first, rhs.first);
        std::swap(lhs.second, rhs.second);
        std::swap(lhs.p_empty, rhs.p_empty);
    }

private:
    bool p_empty = false;
};
}
namespace std
{
template <typename Key, typename T>
struct tuple_size<cpp_utils::containers::nomap_node<Key, T>> : std::integral_constant<size_t, 2>
{
};
template <typename Key, typename T>
struct tuple_element<0, cpp_utils::containers::nomap_node<Key, T>>
{
    using type = Key;
};
template <typename Key, typename T>
struct tuple_element<1, cpp_utils::containers::nomap_node<Key, T>>
{
    using type = cpp_utils::containers::details::base_type_t<T>;
};

}

namespace cpp_utils::containers
{
namespace details
{
// Dereferences the owning pointers of nomap's storage, so iterating a nomap yields its nodes.
template <typename Node, typename Base>
struct node_iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::remove_const_t<Node>;
    using difference_type = std::ptrdiff_t;
    using pointer = Node*;
    using reference = Node&;

    Base base;

    node_iterator() = default;
    explicit node_iterator(Base b) : base { b } { }

    template <typename OtherNode, typename OtherBase>
        requires std::is_convertible_v<OtherBase, Base>
    node_iterator(const node_iterator<OtherNode, OtherBase>& other) : base { other.base }
    {
    }

    [[nodiscard]] reference operator*() const { return **base; }
    [[nodiscard]] pointer operator->() const { return base->get(); }
    [[nodiscard]] reference operator[](difference_type n) const { return *base[n]; }

    node_iterator& operator++()
    {
        ++base;
        return *this;
    }
    node_iterator operator++(int) { return node_iterator { base++ }; }
    node_iterator& operator--()
    {
        --base;
        return *this;
    }
    node_iterator operator--(int) { return node_iterator { base-- }; }
    node_iterator& operator+=(difference_type n)
    {
        base += n;
        return *this;
    }
    node_iterator& operator-=(difference_type n)
    {
        base -= n;
        return *this;
    }

    [[nodiscard]] friend node_iterator operator+(node_iterator it, difference_type n)
    {
        return it += n;
    }
    [[nodiscard]] friend node_iterator operator+(difference_type n, node_iterator it)
    {
        return it += n;
    }
    [[nodiscard]] friend node_iterator operator-(node_iterator it, difference_type n)
    {
        return it -= n;
    }

    template <typename OtherNode, typename OtherBase>
    [[nodiscard]] difference_type operator-(const node_iterator<OtherNode, OtherBase>& other) const
    {
        return base - other.base;
    }
    template <typename OtherNode, typename OtherBase>
    [[nodiscard]] bool operator==(const node_iterator<OtherNode, OtherBase>& other) const
    {
        return base == other.base;
    }
    template <typename OtherNode, typename OtherBase>
    [[nodiscard]] auto operator<=>(const node_iterator<OtherNode, OtherBase>& other) const
    {
        return base <=> other.base;
    }
};
}

// Each node lives in its own allocation, so references to a node stay valid while other
// nodes are inserted or erased: Python bindings hand out raw pointers to them.
template <typename Key, typename T>
struct nomap
{
    using key_type = details::base_type_t<Key>;
    using mapped_type = details::base_type_t<T>;
    using value_type = nomap_node<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = mapped_type&;
    using const_reference = const mapped_type&;

private:
    using storage_t = std::vector<std::unique_ptr<value_type>>;

public:
    using iterator = details::node_iterator<value_type, typename storage_t::iterator>;
    using const_iterator
        = details::node_iterator<const value_type, typename storage_t::const_iterator>;

    nomap() = default;
    nomap(nomap&&) = default;
    nomap(const nomap& other) { *this = other; }
    nomap& operator=(nomap&&) = default;
    nomap& operator=(const nomap& other)
    {
        if (this != &other)
        {
            storage_t nodes;
            nodes.reserve(other.size());
            for (const auto& node : other)
                nodes.push_back(std::make_unique<value_type>(node));
            p_nodes = std::move(nodes);
        }
        return *this;
    }

    inline bool operator==(const nomap& other) const
    {
        if (size() != other.size())
            return false;
        for (const auto& node : *this)
        {
            if (!other.count(node.key()))
                return false;
            if (other[node.key()] != node.mapped())
                return false;
        }
        return true;
    }

    inline bool operator!=(const nomap& other) const { return !(*this == other); }

    [[nodiscard]] inline bool empty() const noexcept { return p_nodes.empty(); }
    [[nodiscard]] inline size_type size() const noexcept { return std::size(p_nodes); }

    [[nodiscard]] inline size_type max_size() const noexcept { return p_nodes.max_size(); }

    inline void clear() noexcept { p_nodes.clear(); }

    [[nodiscard]] inline T& at(const key_type& key)
    {
        auto it = find(key);
        if (it != end())
            return it->second;

        throw std::out_of_range { "Key not found" };
    }

    [[nodiscard]] inline const T& at(const key_type& key) const
    {
        auto it = find(key);
        if (it != cend())
            return it->second;

        throw std::out_of_range { "Key not found" };
    }

    [[nodiscard]] inline T& operator[](const key_type& key)
    {
        auto it = find(key);
        if (it != end())
            return it->second;
        return _append(key, mapped_type {}).second;
    }

    [[nodiscard]] inline T& operator[](key_type&& key)
    {
        auto it = find(key);
        if (it != end())
            return it->second;
        return _append(std::move(key), mapped_type {}).second;
    }

    [[nodiscard]] inline const T& operator[](const key_type& key) const { return this->at(key); }

    [[nodiscard]] inline iterator begin() { return iterator { std::begin(p_nodes) }; }

    [[nodiscard]] inline iterator end() { return iterator { std::end(p_nodes) }; }

    [[nodiscard]] inline auto rbegin() { return std::reverse_iterator { end() }; }

    [[nodiscard]] inline auto rend() { return std::reverse_iterator { begin() }; }

    [[nodiscard]] inline const_iterator begin() const { return cbegin(); }

    [[nodiscard]] inline const_iterator end() const { return cend(); }

    [[nodiscard]] inline auto rbegin() const { return crbegin(); }

    [[nodiscard]] inline auto rend() const { return crend(); }

    [[nodiscard]] inline const_iterator cbegin() const
    {
        return const_iterator { std::cbegin(p_nodes) };
    }

    [[nodiscard]] inline const_iterator cend() const { return const_iterator { std::cend(p_nodes) }; }

    [[nodiscard]] inline auto crbegin() const { return std::reverse_iterator { cend() }; }

    [[nodiscard]] inline auto crend() const { return std::reverse_iterator { cbegin() }; }

    [[nodiscard]] inline value_type extract(const_iterator position)
    {
        if (position == cend())
            return {};
        value_type node = std::move(*position.base->get());
        p_nodes.erase(position.base);
        return node;
    }

    inline iterator erase(const_iterator position)
    {
        if (position == cend())
            return end();
        return iterator { p_nodes.erase(position.base) };
    }

    inline iterator erase(const_iterator first, const_iterator last)
    {
        if (first <= last and last <= cend())
            return iterator { p_nodes.erase(first.base, last.base) };
        return end();
    }

    [[nodiscard]] inline value_type extract(const key_type& key) { return extract(find(key)); }

    friend void swap(nomap& lhs, nomap& rhs) { std::swap(lhs.p_nodes, rhs.p_nodes); }

    [[nodiscard]] iterator find(const key_type& key)
    {
        return std::find_if(begin(), end(), [&key](const auto& node) { return node.first == key; });
    }

    [[nodiscard]] const_iterator find(const key_type& key) const
    {
        return std::find_if(
            cbegin(), cend(), [&key](const auto& node) { return node.first == key; });
    }

    [[nodiscard]] size_type count(const Key& key) const
    {
        if (this->find(key) == this->cend())
            return 0;
        return 1;
    }

    void reserve(size_type count) { p_nodes.reserve(count); }

    template <typename Kt, class... Args>
    std::pair<iterator, bool> emplace(Kt&& key, Args&&... args)
    {
        auto it = find(key);
        if (it == end())
        {
            _append(std::forward<Kt>(key), mapped_type { std::forward<Args>(args)... });
            return { end() - 1, true };
        }
        return { it, false };
    }


private:
    template <typename Kt>
    inline value_type& _append(Kt&& key, mapped_type&& value)
    {
        return *p_nodes.emplace_back(
            std::make_unique<value_type>(std::forward<Kt>(key), std::move(value)));
    }
    storage_t p_nodes;
};
}
