#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <trees/node.hpp>
#include <trees/algorithms.hpp>
#include <cpp_utils.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace cpp_utils::types::detectors;
using namespace cpp_utils::trees;
using namespace cpp_utils::types::pointers;

std::unique_ptr<tree_node<>> make_test_tree()
{
        auto root = std::make_unique<tree_node<>>("root");
        repeat_n([&root](int n){
            root->children.push_back(std::make_unique<tree_node<>>("node "+std::to_string(n)));
        },3);
        return root;
}

const std::string expected=
"root\n"
"├── node 0\n"
"├── node 1\n"
"├── node 2\n";

TEST_CASE( "Print tree", "[trees]" ) {
    std::stringstream ss;
    print_tree(make_test_tree(),ss);
    REQUIRE(expected==ss.str());
}
