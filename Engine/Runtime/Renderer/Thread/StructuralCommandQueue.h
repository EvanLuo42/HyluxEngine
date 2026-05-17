/// @file
/// @brief MPSC command queue for game→render structural mutations and per-frame view
///        submission. Multiple game-thread callers may enqueue; the render thread is the
///        single consumer. Drain semantics: render thread waits for a BeginFrame marker
///        and is handed every command up to and including that marker.

#pragma once

#include "Renderer/Thread/StructuralCommand.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Bounded-blocking MPSC queue. Lock-based for stage 3 (mutex + condvar mirrors
///        the AsyncDispatcher pattern); a lock-free ring is a future-stage optimization.
class StructuralCommandQueue
{
public:
    StructuralCommandQueue() = default;
    ~StructuralCommandQueue() = default;

    StructuralCommandQueue(const StructuralCommandQueue&) = delete;
    StructuralCommandQueue& operator=(const StructuralCommandQueue&) = delete;
    StructuralCommandQueue(StructuralCommandQueue&&) = delete;
    StructuralCommandQueue& operator=(StructuralCommandQueue&&) = delete;

    /// @brief Game thread side. Appends one command to the queue and wakes the consumer.
    void Enqueue(StructuralCommand command);

    /// @brief Render thread side. Blocks until the queue contains at least one BeginFrame
    ///        command or Stop is called. Returns every command up to and including the
    ///        first BeginFrame; the BeginFrame is always the last element when the return
    ///        value is non-empty. Returns an empty vector when stopped before any frame
    ///        arrived.
    [[nodiscard]] std::vector<StructuralCommand> DrainToFrameBoundary();

    /// @brief Render thread side. Non-blocking. Returns every command currently in the
    ///        queue (whether or not it contains a BeginFrame). Empty when the queue is
    ///        empty. Used by the decoupled render-loop path that re-renders the cached
    ///        scene when no new submissions have arrived.
    [[nodiscard]] std::vector<StructuralCommand> TryDrainAll();

    /// @brief Signals the consumer to exit. Wakes any waiter currently blocked in Drain.
    void Stop();

    /// @brief Returns true once Stop has been called.
    [[nodiscard]] bool IsStopped() const noexcept;

    /// @brief Diagnostic accessor; not synchronized beyond a momentary lock acquire.
    [[nodiscard]] std::size_t PendingCount() const noexcept;

private:
    mutable std::mutex            mutex_;
    std::condition_variable       cv_;
    std::deque<StructuralCommand> queue_;
    bool                          stopped_{false};
};

} // namespace Hylux::Renderer
