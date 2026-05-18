/// @file
/// @brief Tests for WorkStealingDeque: owner-only LIFO behavior, multi-thief steal,
///        empty-after-steal race resolution, full-deque rejection.

#include "Core/Async/WorkStealingDeque.h"

#include <doctest/doctest.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

TEST_SUITE_BEGIN("Core::Async");

using namespace Hylux;

namespace
{
struct Task
{
    int id;
};
} // namespace

TEST_CASE("WorkStealingDeque: empty deque returns nullptr from PopBottom and Steal")
{
    WorkStealingDeque<Task*> deque(8);
    CHECK(deque.SizeApprox() == 0);
    CHECK(deque.PopBottom() == nullptr);
    CHECK(deque.Steal() == nullptr);
}

TEST_CASE("WorkStealingDeque: owner LIFO push / pop")
{
    WorkStealingDeque<Task*> deque(8);
    Task a{1}, b{2}, c{3};
    CHECK(deque.PushBottom(&a));
    CHECK(deque.PushBottom(&b));
    CHECK(deque.PushBottom(&c));
    CHECK(deque.SizeApprox() == 3);
    CHECK(deque.PopBottom() == &c);
    CHECK(deque.PopBottom() == &b);
    CHECK(deque.PopBottom() == &a);
    CHECK(deque.PopBottom() == nullptr);
}

TEST_CASE("WorkStealingDeque: steal is FIFO with respect to push order")
{
    WorkStealingDeque<Task*> deque(8);
    Task a{1}, b{2}, c{3};
    deque.PushBottom(&a);
    deque.PushBottom(&b);
    deque.PushBottom(&c);
    CHECK(deque.Steal() == &a);
    CHECK(deque.Steal() == &b);
    CHECK(deque.Steal() == &c);
    CHECK(deque.Steal() == nullptr);
}

TEST_CASE("WorkStealingDeque: full deque rejects PushBottom")
{
    WorkStealingDeque<Task*> deque(4);
    std::vector<Task>        tasks(4);
    for (auto& t : tasks)
    {
        CHECK(deque.PushBottom(&t));
    }
    Task extra{};
    CHECK_FALSE(deque.PushBottom(&extra));
}

TEST_CASE("WorkStealingDeque: concurrent steals each see a unique element, no duplicates, no losses")
{
    constexpr int kCount    = 4096;
    constexpr int kThieves  = 8;
    WorkStealingDeque<Task*> deque(8192);
    std::vector<Task>        tasks(kCount);
    for (int i = 0; i < kCount; ++i)
    {
        tasks[i].id = i;
        CHECK(deque.PushBottom(&tasks[i]));
    }

    std::vector<std::thread>             thieves;
    std::vector<std::vector<Task*>>      stolen(kThieves);
    std::atomic<bool>                    go{false};
    for (int i = 0; i < kThieves; ++i)
    {
        thieves.emplace_back([&, i] {
            while (!go.load(std::memory_order_acquire))
            {
            }
            while (true)
            {
                Task* t = deque.Steal();
                if (t == nullptr)
                {
                    if (deque.SizeApprox() == 0)
                    {
                        break;
                    }
                    continue;
                }
                stolen[i].push_back(t);
            }
        });
    }
    go.store(true, std::memory_order_release);
    for (auto& t : thieves)
    {
        t.join();
    }

    std::unordered_set<Task*> seen;
    for (const auto& bucket : stolen)
    {
        for (Task* t : bucket)
        {
            const auto [_, inserted] = seen.insert(t);
            CHECK(inserted);
        }
    }
    CHECK(seen.size() == static_cast<std::size_t>(kCount));
}

TEST_CASE("WorkStealingDeque: owner pop racing thief steal yields each element exactly once")
{
    constexpr int kCount = 2048;
    WorkStealingDeque<Task*> deque(4096);
    std::vector<Task>        tasks(kCount);
    for (int i = 0; i < kCount; ++i)
    {
        tasks[i].id = i;
    }

    std::atomic<int> pushedSoFar{0};
    std::atomic<int> consumed{0};
    std::unordered_set<int> seenIds;
    std::mutex              seenMu;

    std::thread owner([&] {
        for (int i = 0; i < kCount; ++i)
        {
            while (!deque.PushBottom(&tasks[i]))
            {
                std::this_thread::yield();
            }
            pushedSoFar.store(i + 1, std::memory_order_release);
            if ((i & 3) == 0)
            {
                Task* t = deque.PopBottom();
                if (t != nullptr)
                {
                    {
                        std::lock_guard lock(seenMu);
                        CHECK(seenIds.insert(t->id).second);
                    }
                    consumed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
        while (true)
        {
            Task* t = deque.PopBottom();
            if (t == nullptr)
            {
                break;
            }
            {
                std::lock_guard lock(seenMu);
                CHECK(seenIds.insert(t->id).second);
            }
            consumed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::vector<std::thread> thieves;
    for (int i = 0; i < 4; ++i)
    {
        thieves.emplace_back([&] {
            while (consumed.load(std::memory_order_acquire) < kCount)
            {
                Task* t = deque.Steal();
                if (t == nullptr)
                {
                    std::this_thread::yield();
                    continue;
                }
                {
                    std::lock_guard lock(seenMu);
                    CHECK(seenIds.insert(t->id).second);
                }
                consumed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    owner.join();
    for (auto& t : thieves)
    {
        t.join();
    }
    CHECK(consumed.load() == kCount);
    CHECK(seenIds.size() == static_cast<std::size_t>(kCount));
}

TEST_SUITE_END();
