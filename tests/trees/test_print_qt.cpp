#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <QString>
#include <QTreeWidgetItem>
#include <catch2/catch.hpp>
#include <cpp_utils.hpp>
#include <cpp_utils_qt/cpp_utils_qt.hpp>
#include <memory>
#include <string>
#include <trees/algorithms.hpp>
#include <vector>

using namespace cpp_utils::types::detectors;
using namespace cpp_utils::trees;
using namespace cpp_utils::types::pointers;

auto make_test_tree()
{
    auto root = new QTreeWidgetItem {};
    root->setText(0, "root");
    repeat_n(
        [root](int n) {
            auto child = new QTreeWidgetItem {};
            child->setText(0, QString("node %1").arg(n));
            root->addChild(child);
            repeat_n(
                [root = child](int n) {
                    auto child = new QTreeWidgetItem {};
                    child->setText(0, QString("node %1").arg(n));
                    root->addChild(child);
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
