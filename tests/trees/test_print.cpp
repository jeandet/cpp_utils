#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <cpp_utils.hpp>
#include <memory>
#include <string>
#include <trees/algorithms.hpp>
#include <trees/node.hpp>
#include <vector>

using namespace cpp_utils::types::detectors;
using namespace cpp_utils::trees;
using namespace cpp_utils::types::pointers;

std::unique_ptr<tree_node<>> make_test_tree()
{
    auto root = std::make_unique<tree_node<>>("root");
    repeat_n(
        [&root](int n) {
            root->children.push_back(std::make_unique<tree_node<>>("node " + std::to_string(n)));
            repeat_n(
                [&root=root->children[n]](int n) {
                    root->children.push_back(
                        std::make_unique<tree_node<>>("node " + std::to_string(n)));
                },
                2);
        },
        3);
    return root;
}

const std::string expected = 
R"(root
├── node 0
│   ├── node 0
│   ├── node 1
├── node 1
│   ├── node 0
│   ├── node 1
├── node 2
│   ├── node 0
│   ├── node 1
)";

TEST_CASE("Print tree", "[trees]")
{
    std::stringstream ss;
    print_tree(make_test_tree(), ss);
    REQUIRE(expected == ss.str());
}
