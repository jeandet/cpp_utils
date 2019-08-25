#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <types_detectors.h>
#include <QTreeWidgetItem>
#include <QString>
#include <QSharedPointer>
#include <trees.h>

TEST_CASE( "Qt types detector", "[types]" ) 
{
    REQUIRE(is_qt_tree_item<QTreeWidgetItem>::value);
    REQUIRE(!is_qt_tree_item<tree_node<>>::value); 
    
    REQUIRE(has_toStdString_method<QString>); 
    REQUIRE(!has_toStdString_method<std::string>);
    
    REQUIRE(is_smart_ptr<QSharedPointer<int>>::value);
}
