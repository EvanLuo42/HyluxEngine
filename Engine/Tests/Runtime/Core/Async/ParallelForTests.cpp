/// @file
/// @brief Tests for ParallelFor and TaskGraph::AddParallelFor.

#include "Core/Async/ParallelFor.h"
#include "Core/Async/TaskGraph.h"
#include "Core/Async/WorkerThreadPool.h"

#include <doctest/doctest.h>

#include <array>
#include <atomic>
#include <numeric>
#include <vector>

TEST_SUITE_BEGIN("Core::Async");

using namespace Hylux;

TEST_CASE("ParallelFor: sum of 0..N matches serial")
{
    WorkerThreadPool      pool;
    constexpr std::size_t kN = 10000;
    std::atomic<long long> sum{0};
    ParallelFor(pool.GetExecutor(), 0, kN,
                [&](std::size_t i) { sum.fetch_add(static_cast<long long>(i), std::memory_order_relaxed); })
        .Wait();
    const long long expected = static_cast<long long>(kN) * (kN - 1) / 2;
    CHECK(sum.load() == expected);
}

TEST_CASE("ParallelFor: empty range is a no-op and returns ready future")
{
    WorkerThreadPool pool;
    std::atomic<int> called{0};
    Future<Unit>     f = ParallelFor(pool.GetExecutor(), 5, 5, [&](std::size_t) {
        called.fetch_add(1, std::memory_order_relaxed);
    });
    f.Wait();
    CHECK(called.load() == 0);
    CHECK(f.IsReady());
}

TEST_CASE("ParallelFor: every iteration runs exactly once")
{
    WorkerThreadPool      pool;
    constexpr std::size_t kN = 2048;
    std::vector<int>      hits(kN, 0);
    ParallelFor(pool.GetExecutor(), 0, kN, [&](std::size_t i) { hits[i] = 1; }).Wait();
    int total = 0;
    for (int h : hits)
    {
        total += h;
    }
    CHECK(total == static_cast<int>(kN));
}

TEST_CASE("TaskGraph::AddParallelFor: join node sequences downstream work")
{
    WorkerThreadPool      pool;
    TaskGraph             g;
    constexpr std::size_t kN = 1024;
    std::atomic<int>      iterDone{0};
    std::atomic<int>      finishedBeforeJoin{0};
    std::atomic<bool>     joinFired{false};

    TaskNodeId loop = g.AddParallelFor("loop", {}, 0, kN, [&](std::size_t) {
        iterDone.fetch_add(1, std::memory_order_relaxed);
    });
    g.AddNode("after", std::array{loop}, [&](TaskContext&) {
        finishedBeforeJoin = iterDone.load(std::memory_order_relaxed);
        joinFired.store(true, std::memory_order_release);
    });

    g.Submit(pool.GetExecutor()).Wait();
    CHECK(joinFired.load());
    CHECK(finishedBeforeJoin.load() == static_cast<int>(kN));
}

TEST_SUITE_END();
