/// @file
/// @brief Tests for Hylux::ScopeGuard and HYLUX_DEFER.

#include "Core/Utils/ScopeGuard.h"

#include <doctest/doctest.h>

#include <utility>
#include <vector>

TEST_SUITE_BEGIN("Core::Utils");

using namespace Hylux;

TEST_CASE("ScopeGuard: runs callable at scope exit")
{
    int counter = 0;
    {
        auto guard = MakeScopeGuard([&] { ++counter; });
        CHECK(counter == 0);
        CHECK(guard.IsActive());
    }
    CHECK(counter == 1);
}

TEST_CASE("ScopeGuard: Dismiss skips the callable")
{
    int counter = 0;
    {
        auto guard = MakeScopeGuard([&] { ++counter; });
        guard.Dismiss();
        CHECK_FALSE(guard.IsActive());
    }
    CHECK(counter == 0);
}

TEST_CASE("ScopeGuard: moved-from guard does not double-fire")
{
    int counter = 0;
    {
        auto first = MakeScopeGuard([&] { ++counter; });
        auto second = std::move(first);
        CHECK_FALSE(first.IsActive());
        CHECK(second.IsActive());
    }
    CHECK(counter == 1);
}

TEST_CASE("HYLUX_DEFER: runs at end of enclosing block")
{
    int counter = 0;
    {
        HYLUX_DEFER { counter = 42; };
        CHECK(counter == 0);
    }
    CHECK(counter == 42);
}

TEST_CASE("HYLUX_DEFER: multiple defers run in reverse declaration order")
{
    std::vector<int> order;
    {
        HYLUX_DEFER { order.push_back(1); };
        HYLUX_DEFER { order.push_back(2); };
        HYLUX_DEFER { order.push_back(3); };
    }
    REQUIRE(order.size() == 3u);
    CHECK(order[0] == 3);
    CHECK(order[1] == 2);
    CHECK(order[2] == 1);
}

TEST_CASE("ScopeGuard: callable can mutate captured state")
{
    int value = 5;
    {
        auto guard = MakeScopeGuard([&] { value *= 2; });
        value += 3;
    }
    CHECK(value == 16);
}

TEST_SUITE_END();
