/// @file
/// @brief Multi-threaded work-stealing thread pool. Each worker owns a Chase-Lev deque;
///        non-worker submissions land in a shared inject queue. The pool exposes itself as
///        an IExecutor so engine-level async APIs (Future::Then, TaskGraph, ParallelFor)
///        can post into it through the existing executor abstraction.

#pragma once

#include "Core/Async/IExecutor.h"
#include "Core/Async/WorkStealingDeque.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Hylux
{

class WorkerThreadPool;

/// @brief IExecutor adapter for WorkerThreadPool. Posting from a pool worker routes the
///        work to that worker's local deque (LIFO, cache-warm); posting from elsewhere
///        routes through the shared inject queue.
class WorkerExecutor final : public IExecutor
{
public:
    explicit WorkerExecutor(WorkerThreadPool& pool) noexcept : pool_(pool) {}

    void Post(std::function<void()> work) override;

private:
    WorkerThreadPool& pool_;
};

/// @brief Fixed-size pool of worker threads. Construct, use via Post / WorkerExecutor,
///        destroy to join. Not movable; pin via unique_ptr if you need indirection.
class WorkerThreadPool
{
public:
    /// @brief Tunable parameters. Default workerCount of 0 resolves to
    ///        max(1u, hardware_concurrency() - 2) so the game and render threads each have
    ///        a core to themselves on typical hardware.
    struct Config
    {
        std::size_t workerCount             = 0;
        std::size_t perWorkerDequeCapacity  = 1024;
        int         stealAttemptsBeforeWait = 64;
    };

    explicit WorkerThreadPool(Config cfg = {});
    ~WorkerThreadPool();

    WorkerThreadPool(const WorkerThreadPool&)            = delete;
    WorkerThreadPool& operator=(const WorkerThreadPool&) = delete;

    /// @brief Submits @p work for asynchronous execution. Thread-safe. Empty work is a
    ///        no-op. From a pool worker, pushes to the local deque (with overflow falling
    ///        back to the inject queue); from any other thread, pushes to the inject queue.
    void Post(std::function<void()> work);

    [[nodiscard]] int                GetWorkerCount() const noexcept { return static_cast<int>(workers_.size()); }
    [[nodiscard]] WorkerExecutor&    GetExecutor() noexcept { return executor_; }

private:
    struct Task
    {
        std::function<void()> body;
    };

    struct Worker
    {
        std::unique_ptr<WorkStealingDeque<Task*>> deque;
        std::thread                               thread;
    };

    void  WorkerMain(int selfIndex);
    Task* TryGetWork(int selfIndex);
    void  WakeOne();

    Config                                config_;
    std::vector<std::unique_ptr<Worker>>  workers_;
    std::atomic<bool>                     stop_{false};

    mutable std::mutex                    waitMu_;
    std::condition_variable               waitCv_;

    std::mutex                            injectMu_;
    std::deque<Task*>                     injectQueue_;

    WorkerExecutor                        executor_{*this};
};

} // namespace Hylux
