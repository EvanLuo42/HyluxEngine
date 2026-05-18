/// @file
/// @brief Tests for ThreadAffinity: per-thread role and worker-index TLS.

#include "Core/Concurrency/ThreadAffinity.h"

#include <doctest/doctest.h>

#include <thread>

TEST_SUITE_BEGIN("Core::Concurrency");

using namespace Hylux::Concurrency;

TEST_CASE("ThreadAffinity: default role is Unknown")
{
    std::thread t([] {
        CHECK(GetCurrentThreadRole() == ThreadRole::Unknown);
        CHECK(GetCurrentWorkerIndex() == -1);
    });
    t.join();
}

TEST_CASE("ThreadAffinity: each thread carries its own role")
{
    ThreadRole observedA = ThreadRole::Unknown;
    ThreadRole observedB = ThreadRole::Unknown;

    std::thread a([&] {
        RegisterCurrentThreadRole(ThreadRole::GameThread);
        observedA = GetCurrentThreadRole();
    });
    std::thread b([&] {
        RegisterCurrentThreadRole(ThreadRole::RenderThread);
        observedB = GetCurrentThreadRole();
    });
    a.join();
    b.join();

    CHECK(observedA == ThreadRole::GameThread);
    CHECK(observedB == ThreadRole::RenderThread);
}

TEST_CASE("ThreadAffinity: worker index round-trips")
{
    std::thread t([] {
        RegisterCurrentThreadRole(ThreadRole::Worker);
        SetCurrentWorkerIndex(3);
        CHECK(GetCurrentThreadRole() == ThreadRole::Worker);
        CHECK(GetCurrentWorkerIndex() == 3);
    });
    t.join();
}

TEST_SUITE_END();
