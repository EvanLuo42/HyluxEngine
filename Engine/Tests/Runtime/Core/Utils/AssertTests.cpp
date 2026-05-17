/// @file
/// @brief Tests for Hylux::Diagnostics assertion macros.

#include "Core/Utils/Assert.h"

#include <doctest/doctest.h>

#include <cstring>
#include <string>

TEST_SUITE_BEGIN("Core::Utils");

namespace
{

struct CapturedFailure
{
    bool        fired{false};
    std::string file;
    int         line{0};
    std::string expr;
    std::string message;
};

CapturedFailure g_lastFailure;

void RecordingHandler(const char* file, int line, const char* expr, const char* message)
{
    g_lastFailure.fired   = true;
    g_lastFailure.file    = file != nullptr ? file : "";
    g_lastFailure.line    = line;
    g_lastFailure.expr    = expr != nullptr ? expr : "";
    g_lastFailure.message = message != nullptr ? message : "";
}

struct HandlerScope
{
    Hylux::Diagnostics::AssertionHandler previous{nullptr};

    HandlerScope() noexcept
        : previous(Hylux::Diagnostics::SetAssertionHandler(&RecordingHandler))
    {
        g_lastFailure = CapturedFailure{};
    }
    ~HandlerScope()
    {
        Hylux::Diagnostics::SetAssertionHandler(previous);
    }
};

} // namespace

TEST_CASE("SetAssertionHandler: returns previous handler; nullptr restores default")
{
    using namespace Hylux::Diagnostics;
    AssertionHandler original = GetAssertionHandler();

    AssertionHandler returned = SetAssertionHandler(&RecordingHandler);
    CHECK(returned == original);
    CHECK(GetAssertionHandler() == &RecordingHandler);

    AssertionHandler returnedAgain = SetAssertionHandler(nullptr);
    CHECK(returnedAgain == &RecordingHandler);
    CHECK(GetAssertionHandler() == &DefaultAssertionHandler);

    // Restore so other tests are unaffected.
    SetAssertionHandler(original);
}

TEST_CASE("HYLUX_CHECK: fires the handler on falsy expressions")
{
    HandlerScope scope;
    HYLUX_CHECK(1 == 1);
    CHECK_FALSE(g_lastFailure.fired);

    HYLUX_CHECK(1 == 2);
    REQUIRE(g_lastFailure.fired);
    CHECK(g_lastFailure.expr == "1 == 2");
    CHECK(g_lastFailure.line > 0);
}

TEST_CASE("HYLUX_CHECK_MSG: passes the literal message to the handler")
{
    HandlerScope scope;
    HYLUX_CHECK_MSG(false, "explanatory text");
    REQUIRE(g_lastFailure.fired);
    CHECK(g_lastFailure.message == "explanatory text");
}

TEST_CASE("HYLUX_VERIFY: evaluates the expression even in Release configurations")
{
    HandlerScope scope;
    int sideEffects = 0;
    auto bump      = [&] { ++sideEffects; return true; };
    HYLUX_VERIFY(bump());
    CHECK(sideEffects == 1);
    CHECK_FALSE(g_lastFailure.fired);
}

TEST_CASE("HYLUX_VERIFY: dev builds report failure; release builds discard the result")
{
    HandlerScope scope;
    int          sideEffects = 0;
    auto         falsy       = [&] { ++sideEffects; return false; };
    HYLUX_VERIFY(falsy());
    CHECK(sideEffects == 1);
    if constexpr (Hylux::Diagnostics::kBuildIsDev)
    {
        CHECK(g_lastFailure.fired);
    }
    else
    {
        CHECK_FALSE(g_lastFailure.fired);
    }
}

TEST_CASE("HYLUX_ASSERT: only fires in Debug builds")
{
    HandlerScope scope;
    HYLUX_ASSERT(false);
    if constexpr (Hylux::Diagnostics::kBuildIsDebug)
    {
        CHECK(g_lastFailure.fired);
    }
    else
    {
        CHECK_FALSE(g_lastFailure.fired);
    }
}

TEST_CASE("HYLUX_ASSERT: does NOT evaluate the expression when compiled out")
{
    HandlerScope scope;
    int sideEffects = 0;
    auto increment  = [&] { ++sideEffects; return true; };
    HYLUX_ASSERT(increment());
    if constexpr (Hylux::Diagnostics::kBuildIsDebug)
    {
        CHECK(sideEffects == 1);
    }
    else
    {
        CHECK(sideEffects == 0);
    }
}

TEST_SUITE_END();
