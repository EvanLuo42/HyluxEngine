/// @file
/// @brief WorkerThreadPool implementation. Each worker runs WorkerMain: pop own deque,
///        steal from a random victim, drain inject queue, sleep on condvar.

#include "Core/Async/WorkerThreadPool.h"

#include "Core/Concurrency/ThreadAffinity.h"

#include <algorithm>
#include <random>
#include <utility>

namespace Hylux
{

namespace
{
thread_local WorkerThreadPool* tlsOwningPool_ = nullptr;

std::size_t ResolveWorkerCount(std::size_t requested) noexcept
{
    if (requested > 0)
    {
        return requested;
    }
    const unsigned hw = std::thread::hardware_concurrency();
    if (hw <= 3)
    {
        return 1;
    }
    return static_cast<std::size_t>(hw - 2);
}

std::size_t NextPowerOfTwo(std::size_t v) noexcept
{
    if (v < 2)
    {
        return 2;
    }
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
}
} // namespace

void WorkerExecutor::Post(std::function<void()> work)
{
    pool_.Post(std::move(work));
}

WorkerThreadPool::WorkerThreadPool(Config cfg) : config_(std::move(cfg))
{
    const std::size_t count    = ResolveWorkerCount(config_.workerCount);
    const std::size_t capacity = NextPowerOfTwo(config_.perWorkerDequeCapacity);
    config_.workerCount             = count;
    config_.perWorkerDequeCapacity  = capacity;

    workers_.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        auto w   = std::make_unique<Worker>();
        w->deque = std::make_unique<WorkStealingDeque<Task*>>(capacity);
        workers_.push_back(std::move(w));
    }
    for (std::size_t i = 0; i < count; ++i)
    {
        workers_[i]->thread = std::thread(&WorkerThreadPool::WorkerMain, this, static_cast<int>(i));
    }
}

WorkerThreadPool::~WorkerThreadPool()
{
    stop_.store(true, std::memory_order_release);
    {
        std::lock_guard lock(waitMu_);
        waitCv_.notify_all();
    }
    for (auto& w : workers_)
    {
        if (w->thread.joinable())
        {
            w->thread.join();
        }
    }

    auto drainAndDelete = [](std::deque<Task*>& q) {
        while (!q.empty())
        {
            delete q.front();
            q.pop_front();
        }
    };
    {
        std::lock_guard lock(injectMu_);
        drainAndDelete(injectQueue_);
    }
    for (auto& w : workers_)
    {
        while (Task* t = w->deque->PopBottom())
        {
            delete t;
        }
    }
}

void WorkerThreadPool::Post(std::function<void()> work)
{
    if (!work)
    {
        return;
    }
    auto* task = new Task{std::move(work)};

    const int selfIdx = Concurrency::GetCurrentWorkerIndex();
    if (tlsOwningPool_ == this && selfIdx >= 0 && selfIdx < static_cast<int>(workers_.size()))
    {
        if (workers_[static_cast<std::size_t>(selfIdx)]->deque->PushBottom(task))
        {
            WakeOne();
            return;
        }
    }

    {
        std::lock_guard lock(injectMu_);
        injectQueue_.push_back(task);
    }
    WakeOne();
}

void WorkerThreadPool::WakeOne()
{
    std::lock_guard lock(waitMu_);
    waitCv_.notify_one();
}

WorkerThreadPool::Task* WorkerThreadPool::TryGetWork(int selfIndex)
{
    if (Task* t = workers_[static_cast<std::size_t>(selfIndex)]->deque->PopBottom())
    {
        return t;
    }

    const int n = static_cast<int>(workers_.size());
    if (n > 1)
    {
        thread_local std::minstd_rand rng{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, n - 2);
        for (int attempt = 0; attempt < config_.stealAttemptsBeforeWait; ++attempt)
        {
            int victim = dist(rng);
            if (victim >= selfIndex)
            {
                ++victim;
            }
            if (Task* t = workers_[static_cast<std::size_t>(victim)]->deque->Steal())
            {
                return t;
            }
        }
    }

    {
        std::lock_guard lock(injectMu_);
        if (!injectQueue_.empty())
        {
            Task* t = injectQueue_.front();
            injectQueue_.pop_front();
            return t;
        }
    }
    return nullptr;
}

void WorkerThreadPool::WorkerMain(int selfIndex)
{
    Concurrency::RegisterCurrentThreadRole(Concurrency::ThreadRole::Worker);
    Concurrency::SetCurrentWorkerIndex(selfIndex);
    tlsOwningPool_ = this;

    while (true)
    {
        Task* t = TryGetWork(selfIndex);
        if (t != nullptr)
        {
            try
            {
                t->body();
            }
            catch (...)
            {
            }
            delete t;
            continue;
        }

        if (stop_.load(std::memory_order_acquire))
        {
            break;
        }

        std::unique_lock lock(waitMu_);
        waitCv_.wait(lock, [this, selfIndex] {
            if (stop_.load(std::memory_order_acquire))
            {
                return true;
            }
            for (const auto& w : workers_)
            {
                if (w->deque->SizeApprox() > 0)
                {
                    return true;
                }
            }
            std::lock_guard injectLock(injectMu_);
            return !injectQueue_.empty();
        });
    }
}

} // namespace Hylux
