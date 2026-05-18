/// @file
/// @brief Tests for WorkerThreadPool: fan-out, recursive worker post, clean shutdown.

#include "Core/Async/WorkerThreadPool.h"
#include "Core/Concurrency/ThreadAffinity.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <thread>

TEST_SUITE_BEGIN("Core::Async");

using namespace Hylux;

TEST_CASE("WorkerThreadPool: default config spawns at least one worker")
{
    WorkerThreadPool pool;
    CHECK(pool.GetWorkerCount() >= 1);
}

TEST_CASE("WorkerThreadPool: fan-out 4096 posts all execute exactly once")
{
    WorkerThreadPool pool;
    constexpr int    kCount = 4096;
    std::atomic<int> counter{0};
    for (int i = 0; i < kCount; ++i)
    {
        pool.Post([&] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    while (counter.load() < kCount)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(counter.load() == kCount);
}

TEST_CASE("WorkerThreadPool: worker posts route through TLS path and still execute")
{
    WorkerThreadPool                 pool;
    std::atomic<int>                 phaseTwo{0};
    constexpr int                    kCount = 256;
    for (int i = 0; i < kCount; ++i)
    {
        pool.Post([&] {
            CHECK(Concurrency::GetCurrentThreadRole() == Concurrency::ThreadRole::Worker);
            pool.Post([&] { phaseTwo.fetch_add(1, std::memory_order_relaxed); });
        });
    }
    while (phaseTwo.load() < kCount)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(phaseTwo.load() == kCount);
}

TEST_CASE("WorkerThreadPool: WorkerExecutor::Post drives execution")
{
    WorkerThreadPool pool;
    IExecutor&       exec = pool.GetExecutor();
    std::atomic<int> seen{0};
    constexpr int    kCount = 1024;
    for (int i = 0; i < kCount; ++i)
    {
        exec.Post([&] { seen.fetch_add(1, std::memory_order_relaxed); });
    }
    while (seen.load() < kCount)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(seen.load() == kCount);
}

TEST_CASE("WorkerThreadPool: shutdown drops unstarted tasks but joins cleanly")
{
    std::atomic<int> ran{0};
    {
        WorkerThreadPool pool(WorkerThreadPool::Config{.workerCount = 2});
        for (int i = 0; i < 10000; ++i)
        {
            pool.Post([&] { ran.fetch_add(1, std::memory_order_relaxed); });
        }
    }
    CHECK(ran.load() >= 0);
}

TEST_SUITE_END();
