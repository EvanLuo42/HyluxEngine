/// @file
/// @brief Tests for TaskGraph: linear chain, diamond, fan-out, pinning, cancellation.

#include "Core/Async/TaskGraph.h"
#include "Core/Async/WorkerThreadPool.h"

#include <doctest/doctest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("Core::Async");

using namespace Hylux;

TEST_CASE("TaskGraph: empty graph resolves immediately")
{
    WorkerThreadPool pool;
    TaskGraph        g;
    Future<Unit>     f = g.Submit(pool.GetExecutor());
    f.Wait();
    CHECK(f.IsReady());
}

TEST_CASE("TaskGraph: linear chain runs in dependency order")
{
    WorkerThreadPool        pool;
    TaskGraph               g;
    std::vector<int>        log;
    std::mutex              logMu;

    auto append = [&](int id) {
        std::lock_guard lock(logMu);
        log.push_back(id);
    };

    TaskNodeId a = g.AddNode("a", [&](TaskContext&) { append(1); });
    TaskNodeId b = g.AddNode("b", std::array{a}, [&](TaskContext&) { append(2); });
    TaskNodeId c = g.AddNode("c", std::array{b}, [&](TaskContext&) { append(3); });
    (void)c;

    g.Submit(pool.GetExecutor()).Wait();

    CHECK(log == std::vector{1, 2, 3});
}

TEST_CASE("TaskGraph: diamond — a -> b,c -> d ; b and c run after a, d runs after both")
{
    WorkerThreadPool pool;
    TaskGraph        g;
    std::atomic<int> phaseAtA{0};
    std::atomic<int> phaseAtB{0};
    std::atomic<int> phaseAtC{0};
    std::atomic<int> phaseAtD{0};
    std::atomic<int> counter{0};

    TaskNodeId a = g.AddNode("a", [&](TaskContext&) {
        phaseAtA = counter.fetch_add(1, std::memory_order_relaxed);
    });
    TaskNodeId b = g.AddNode("b", std::array{a}, [&](TaskContext&) {
        phaseAtB = counter.fetch_add(1, std::memory_order_relaxed);
    });
    TaskNodeId c = g.AddNode("c", std::array{a}, [&](TaskContext&) {
        phaseAtC = counter.fetch_add(1, std::memory_order_relaxed);
    });
    g.AddNode("d", std::array{b, c}, [&](TaskContext&) {
        phaseAtD = counter.fetch_add(1, std::memory_order_relaxed);
    });

    g.Submit(pool.GetExecutor()).Wait();

    CHECK(phaseAtA.load() == 0);
    CHECK(phaseAtB.load() > phaseAtA.load());
    CHECK(phaseAtC.load() > phaseAtA.load());
    CHECK(phaseAtD.load() == 3);
}

TEST_CASE("TaskGraph: fan-out 1k independent nodes all run")
{
    WorkerThreadPool pool;
    TaskGraph        g;
    constexpr int    kCount = 1024;
    std::atomic<int> seen{0};
    for (int i = 0; i < kCount; ++i)
    {
        g.AddNode("n", [&](TaskContext&) { seen.fetch_add(1, std::memory_order_relaxed); });
    }
    g.Submit(pool.GetExecutor()).Wait();
    CHECK(seen.load() == kCount);
}

TEST_CASE("TaskGraph: cancellation skips bodies but drains the graph")
{
    WorkerThreadPool  pool;
    TaskGraph         g;
    CancellationSource src;
    std::atomic<int>   ran{0};
    constexpr int      kCount = 64;
    for (int i = 0; i < kCount; ++i)
    {
        g.AddNode("n", [&](TaskContext& ctx) {
            if (ctx.Cancellation().IsCanceled())
            {
                return;
            }
            ran.fetch_add(1, std::memory_order_relaxed);
        });
    }
    src.Cancel();
    Future<Unit> f = g.Submit(pool.GetExecutor(), src.GetToken());
    f.Wait();
    CHECK(f.IsReady());
    CHECK(ran.load() == 0);
}

TEST_CASE("TaskGraph: AddNodeOn routes node to the pinned executor")
{
    WorkerThreadPool pool;
    QueuedExecutor   pinned;

    TaskGraph        g;
    std::atomic<int> pinnedExecuted{0};
    std::atomic<int> defaultExecuted{0};
    g.AddNodeOn(&pinned, "pinned", {}, [&](TaskContext&) {
        pinnedExecuted.fetch_add(1, std::memory_order_relaxed);
    });
    g.AddNode("default", [&](TaskContext&) {
        defaultExecuted.fetch_add(1, std::memory_order_relaxed);
    });

    Future<Unit> f = g.Submit(pool.GetExecutor());

    while (pinned.PendingCount() == 0 && defaultExecuted.load() == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(pinned.Drain() == 1);
    f.Wait();
    CHECK(pinnedExecuted.load() == 1);
    CHECK(defaultExecuted.load() == 1);
}

TEST_SUITE_END();
