#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <QExplicitlySharedDataPointer>
#include <QScopedPointer>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>
#include <QTreeWidgetItem>
#include <QWeakPointer>
#include <catch2/catch.hpp>
#include <types/pointers.hpp>
#include <cpp_utils_qt/cpp_utils_qt.hpp>


TEST_CASE("Qt types detector", "[types]")
{

    using namespace cpp_utils::types::detectors;
    using namespace cpp_utils::trees;

    REQUIRE(is_qt_tree_item<QTreeWidgetItem>::value);
    REQUIRE(!is_qt_tree_item<tree_node<>>::value);

    REQUIRE(has_toStdString_method_v<QString>);
    REQUIRE(!has_toStdString_method_v<std::string>);

    using namespace cpp_utils::types::pointers;

#define _PTR_TEST(name, type)                                                                      \
    auto test_##name##_##type##_lambda = []() -> bool {                                            \
        if constexpr (std::is_same_v<name<int>, type<int>>)                                        \
            return is_##type##_v<name<int>>;                                                       \
        return !is_##type##_v<name<int>>;                                                          \
    };                                                                                             \
    REQUIRE(test_##name##_##type##_lambda());


#define PTR_TEST(name)                                                                             \
    _PTR_TEST(name, QSharedDataPointer);                                                           \
    _PTR_TEST(name, QSharedPointer);                                                               \
    _PTR_TEST(name, QWeakPointer);                                                                 \
    _PTR_TEST(name, QExplicitlySharedDataPointer);                                                 \
    _PTR_TEST(name, QScopedPointer);                                                               \
    _PTR_TEST(name, QScopedArrayPointer);                                                          \
    REQUIRE(is_qt_smart_ptr_v<name<int>>);                                                         \
    REQUIRE(is_smart_ptr_v<name<int>>);


    // list taken here https://wiki.qt.io/Smart_Pointers
    PTR_TEST(QSharedDataPointer)
    PTR_TEST(QSharedPointer)
    PTR_TEST(QWeakPointer)
    PTR_TEST(QExplicitlySharedDataPointer)
    PTR_TEST(QScopedPointer)
    PTR_TEST(QScopedArrayPointer)
}
