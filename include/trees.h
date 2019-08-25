#pragma once
#include "cpp_utils.h"
#include "types_detectors.h"

#include <algorithm>
#include <iomanip>
#include <iostream>


template<typename string_type=std::string>
struct tree_node
{
    using iterator_t = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().begin());
    using const_iterator_t = decltype(std::declval<std::vector<std::unique_ptr<tree_node>>>().cbegin());
    
    string_type name;
    tree_node* parent;
    std::vector<std::unique_ptr<tree_node<string_type>>> children;
    
    tree_node(const string_type& name, tree_node* parent=nullptr)
    :name{name}, parent{parent}
    {}

    iterator_t begin(){return children.begin();}
    iterator_t end(){return children.end();}
    const_iterator_t begin()const {return children.cbegin();}
    const_iterator_t end()const {return children.cend();}
    const_iterator_t cbegin()const {return children.cbegin();}
    const_iterator_t cend()const {return children.cend();}
};

namespace{
HAS_MEMBER(name)
HAS_METHOD(name)
HAS_METHOD(text)
};

template<typename T> std::string _get_name(const T& item)
{
  if constexpr(has_name_method<T>) { return _get_name(item.name()); }
  else if constexpr(has_text_method<T>)
  {
    return _get_name(item.text(0));
  }
  else if constexpr(has_toStdString_method<T>)
  {
    return _get_name(item.toStdString());
  }
  else if constexpr (has_name_member_object<T>)
  {
      return to_std_string(item.name);
  }
  else
  {
    return item;
  }
}

template<typename T, typename U>
void _print_tree(const T& tree, int indent_increment, int indent_lvl, U& ostream)
{
  auto print_lambda = [&indent_lvl, &indent_increment, &ostream](const auto& node) {
    indent_lvl * [indent_increment, &ostream](){ostream <<"│"<< std::string(indent_increment, ' ');};
    ostream << "├";
    (indent_increment-1) * [&ostream](){ostream <<"─";};
    ostream << " "<< _get_name(to_value(node)) << std::endl;
    _print_tree(to_value(node), indent_increment, indent_lvl+1, ostream);
  };
  if constexpr(is_qt_tree_item<T>::value)
  {
    for(int i = 0; i < tree.childCount(); i++)
    {
      print_lambda(tree.child(i));
    }
  }
  else
  {
    std::for_each(std::cbegin(tree), std::cend(tree), print_lambda);
  }
}

template<typename T, typename U=decltype(std::cout), int indent_increment=3 >
void print_tree(const T& tree, U& ostream=std::cout)
{
    ostream << _get_name(to_value(tree)) << std::endl;
  _print_tree(to_value(tree), indent_increment, 0, ostream);
}
