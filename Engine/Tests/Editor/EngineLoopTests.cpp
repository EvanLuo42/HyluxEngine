/// @file
/// @brief Tests for the threaded EngineLoop wrapper: init future resolves, queued
///        commands execute in submission order, Stop joins cleanly.

#include "Core/Async/Future.h"
#include "Engine/Engine.h"
#include "Engine/EngineLoop.h"
#include "Engine/ISubsystem.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace Hylux;

namespace
{

class NoopSubsystem final : public ISubsystem
{
public:
    void Initialize(Engine&) override { initialized = true; }
    void Shutdown() override { shuttingDown = true; }

    bool initialized   = false;
    bool shuttingDown  = false;
};

} // namespace

TEST_CASE("EngineLoop Start resolves init future and Stop joins")
{
    Engine engine;
    auto*  sub = engine.RegisterSubsystem<NoopSubsystem>();

    EngineLoop loop(engine, EngineLoop::Config{.targetHz = 240.0f});
    Future<bool> initFut = loop.Start();
    CHECK(initFut.Wait() == true);
    CHECK(sub->initialized);

    loop.Stop();
    CHECK(sub->shuttingDown);
    CHECK_FALSE(loop.IsRunning());
}

TEST_CASE("EngineLoop drains commands in submission order")
{
    Engine engine;
    engine.RegisterSubsystem<NoopSubsystem>();

    EngineLoop loop(engine, EngineLoop::Config{.targetHz = 240.0f});
    REQUIRE(loop.Start().Wait());

    std::mutex             mu;
    std::vector<int>       seen;
    constexpr int          kCount = 16;
    for (int i = 0; i < kCount; ++i)
    {
        loop.SubmitCommand([i, &mu, &seen](Engine&) {
            std::lock_guard lock(mu);
            seen.push_back(i);
        });
    }

    for (int i = 0; i < 200; ++i)
    {
        {
            std::lock_guard lock(mu);
            if (seen.size() == static_cast<std::size_t>(kCount))
            {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    {
        std::lock_guard lock(mu);
        REQUIRE(seen.size() == static_cast<std::size_t>(kCount));
        for (int i = 0; i < kCount; ++i)
        {
            CHECK(seen[static_cast<std::size_t>(i)] == i);
        }
    }

    loop.Stop();
}

TEST_CASE("EngineLoop Invoke returns a resolved future")
{
    Engine engine;
    engine.RegisterSubsystem<NoopSubsystem>();

    EngineLoop loop(engine);
    REQUIRE(loop.Start().Wait());

    Future<int> fut = loop.Invoke([](Engine&) { return 42; });
    CHECK(fut.Wait() == 42);

    loop.Stop();
}
