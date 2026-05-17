/// @file
/// @brief Tests for MenuRegistry path parsing, ordering, and trigger dispatch.

#include "Menu/MenuRegistry.h"

#include <doctest/doctest.h>

#include <QVariantMap>

using namespace Hylux::Editor;

TEST_CASE("MenuRegistry parses slash-separated paths")
{
    MenuRegistry registry;
    registry.Register(MenuActionDesc{.path = "File/Open"});
    registry.Register(MenuActionDesc{.path = "File/Save"});
    registry.Register(MenuActionDesc{.path = "Help/About"});

    const QVariantList top = registry.topLevelMenus();
    REQUIRE(top.size() == 2);
    CHECK(top[0].toMap().value("title").toString() == "File");
    CHECK(top[1].toMap().value("title").toString() == "Help");

    const QVariantList fileChildren = registry.itemsAt("File");
    REQUIRE(fileChildren.size() == 2);
    CHECK(fileChildren[0].toMap().value("title").toString() == "Open");
    CHECK(fileChildren[1].toMap().value("title").toString() == "Save");
}

TEST_CASE("MenuRegistry trigger invokes the leaf callback")
{
    MenuRegistry registry;
    bool         fired = false;
    registry.Register(MenuActionDesc{
        .path = "Tools/Run", .run = [&fired]() { fired = true; }});

    registry.trigger("Tools/Run");
    CHECK(fired);
}

TEST_CASE("MenuRegistry skips disabled leaves on trigger")
{
    MenuRegistry registry;
    bool         fired = false;
    registry.Register(MenuActionDesc{.path      = "Tools/Run",
                                     .run       = [&fired]() { fired = true; },
                                     .isEnabled = []() { return false; }});

    registry.trigger("Tools/Run");
    CHECK_FALSE(fired);
}
