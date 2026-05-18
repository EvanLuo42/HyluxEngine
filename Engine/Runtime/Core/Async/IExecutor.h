/// @file
/// @brief IExecutor — minimal "where does this callable run" abstraction used by
///        Future::Then continuations and by any subsystem that wants to post work
///        across a thread boundary. Two built-in implementations: InlineExecutor runs
///        the work immediately on the caller's thread (the default for fast paths /
///        already-ready futures); QueuedExecutor stores work for a later Drain() call,
///        which is what subsystems use to marshal continuations back to a specific
///        thread (e.g. game thread or render thread).

#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace Hylux
{

/// @brief Posts work for execution. The implementation decides when and on which thread
///        the work runs. The Future API uses this to route Then() continuations.
class IExecutor
{
public:
    virtual ~IExecutor() = default;

    /// @brief Submits @p work for execution. The work may run synchronously (inline) or
    ///        deferred — callers must not assume either. Implementations must invoke @p
    ///        work exactly once.
    virtual void Post(std::function<void()> work) = 0;
};

/// @brief Executor that runs work synchronously on the calling thread. Useful as the
///        default for Future continuations when the caller doesn't care about marshaling.
class InlineExecutor final : public IExecutor
{
public:
    void Post(std::function<void()> work) override
    {
        if (work)
        {
            work();
        }
    }

    /// @brief Singleton accessor. Address-stable — safe to compare against and to hold
    ///        long-lived references to.
    [[nodiscard]] static InlineExecutor& Instance() noexcept
    {
        static InlineExecutor instance;
        return instance;
    }
};

/// @brief Executor that queues work for later draining by whoever owns the executor.
///        Typical pattern: a subsystem owns one of these as its "post back to my thread"
///        target and calls Drain() each tick on its own thread.
class QueuedExecutor final : public IExecutor
{
public:
    void Post(std::function<void()> work) override
    {
        if (!work)
        {
            return;
        }
        std::lock_guard lock(mutex_);
        pending_.push_back(std::move(work));
    }

    /// @brief Runs every pending work item on the calling thread. Items posted while
    ///        Drain is running are deferred to the next Drain call. Returns the number
    ///        of items executed.
    std::size_t Drain()
    {
        std::vector<std::function<void()>> local;
        {
            std::lock_guard lock(mutex_);
            local.swap(pending_);
        }
        for (auto& work : local)
        {
            work();
        }
        return local.size();
    }

    /// @brief Number of items currently queued. Snapshot-only — racy by definition.
    [[nodiscard]] std::size_t PendingCount() const noexcept
    {
        std::lock_guard lock(mutex_);
        return pending_.size();
    }

private:
    mutable std::mutex                 mutex_;
    std::vector<std::function<void()>> pending_;
};

} // namespace Hylux
