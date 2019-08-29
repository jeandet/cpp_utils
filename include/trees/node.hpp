#pragma once
#include <algorithm>
#include <iomanip>
#include <iostream>
#include "../types/pointers.hpp"

namespace cpp_utils::trees
{
    
    template<typename node_t>
    inline auto&  child(node_t&& node, int index)
    {
        return cpp_utils::types::pointers::to_value_ref(node.children[index]);
    }
    
    template<typename node_t>
    std::size_t inline children_count(node_t&& node)
    {
        return std::size(node.children);
    }
    
    template<typename node_t>
    auto inline name(node_t&& node)
    {
        return node.name;
    }
    
template <typename string_type = std::string>
struct tree_node
{
    using iterator_t = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().begin());
    using const_iterator_t
        = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().cbegin());

    string_type name;
    tree_node* parent;
    std::vector<std::unique_ptr<tree_node<string_type>>> children;

    tree_node(const string_type& name, tree_node* parent = nullptr)
            : name { name }, parent { parent }
    {
    }

    iterator_t begin() { return children.begin(); }
    iterator_t end() { return children.end(); }
    const_iterator_t begin() const { return children.cbegin(); }
    const_iterator_t end() const { return children.cend(); }
    const_iterator_t cbegin() const { return children.cbegin(); }
    const_iterator_t cend() const { return children.cend(); }
};
}
