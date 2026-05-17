/// @file
/// @brief Minimal completion primitive used as the loader / async-task return type so
///        callers commit to an async-shaped API even while v1 implementations fulfill
///        synchronously. Smaller and cheaper than std::future: no heap allocation in the
///        already-ready fast path, shared state participates in the intrusive Ref<>
///        system instead of std::shared_ptr.

#pragma once

#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux
{

namespace Detail
{

/// @brief Shared state behind a Future / Promise pair. Holds the payload, the
///        synchronisation primitives, and the registered continuations. Refcounted so the
///        future and promise can share ownership without dragging in std::shared_ptr.
template<typename T>
class FutureState final : public RefCounted
{
public:
    using Callback = std::function<void(T&)>;

    FutureState() noexcept = default;

    explicit FutureState(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : value_(std::move(value))
    {
        ready_.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool IsReady() const noexcept
    {
        return ready_.load(std::memory_order_acquire);
    }

    T& Wait()
    {
        if (!IsReady())
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return ready_.load(std::memory_order_acquire); });
        }
        return *value_;
    }

    /// @brief Fulfils the state, copies the callback list, releases the lock, and invokes
    ///        callbacks outside the lock so continuations cannot deadlock against producers
    ///        already holding their own locks.
    void Set(T value)
    {
        std::vector<Callback> toRun;
        {
            std::lock_guard lock(mutex_);
            value_.emplace(std::move(value));
            ready_.store(true, std::memory_order_release);
            toRun.swap(callbacks_);
        }
        cv_.notify_all();
        for (auto& cb : toRun)
        {
            cb(*value_);
        }
    }

    /// @brief Registers a continuation. If the state is already ready, the callback is
    ///        invoked synchronously on the caller's thread before this method returns.
    ///        Otherwise it runs from whichever thread eventually calls Set().
    void AddContinuation(Callback callback)
    {
        {
            std::lock_guard lock(mutex_);
            if (!ready_.load(std::memory_order_acquire))
            {
                callbacks_.push_back(std::move(callback));
                return;
            }
        }
        callback(*value_);
    }

private:
    mutable std::mutex      mutex_;
    std::condition_variable cv_;
    std::optional<T>        value_;
    std::vector<Callback>   callbacks_;
    std::atomic<bool>       ready_{false};
};

} // namespace Detail

template<typename T> class Promise;

/// @brief Read-only handle to a value that will be produced (possibly synchronously, in
///        which case the future is already-ready on construction). Multiple Futures can
///        share the same state; the value lives in the shared state for its full
///        lifetime, so Wait() may be called more than once and from more than one thread.
template<typename T>
class Future
{
public:
    using ElementType = T;

    Future() = default;

    /// @brief Convenience factory for the synchronous-fulfilled case. Allocates the
    ///        shared state in already-ready form so Wait() never blocks and AddContinuation
    ///        invokes its callback inline.
    [[nodiscard]] static Future MakeReady(T value)
    {
        Future f;
        f.state_ = MakeRef<Detail::FutureState<T>>(std::move(value));
        return f;
    }

    /// @brief Convenience for "load failed". Requires T to be default-constructible (e.g.
    ///        Ref<X> defaults to null). Callers detect failure by inspecting the resolved
    ///        value; there is no separate failure channel.
    [[nodiscard]] static Future MakeFailed()
    {
        static_assert(std::is_default_constructible_v<T>,
                      "Future<T>::MakeFailed requires T to be default-constructible");
        return MakeReady(T{});
    }

    [[nodiscard]] bool IsValid() const noexcept { return static_cast<bool>(state_); }
    [[nodiscard]] bool IsReady() const noexcept { return state_ && state_->IsReady(); }

    /// @brief Blocks until the value is set, then returns a reference into the shared
    ///        state. The reference remains valid for the lifetime of the Future (and any
    ///        other Future sharing the same state).
    T& Wait() { return state_->Wait(); }

    /// @brief Registers a continuation. If the future is already ready, the callback runs
    ///        synchronously on the calling thread before this returns; otherwise it runs
    ///        from the thread that eventually calls Promise::Set.
    void Then(std::function<void(T&)> callback)
    {
        state_->AddContinuation(std::move(callback));
    }

private:
    friend class Promise<T>;

    explicit Future(Ref<Detail::FutureState<T>> state) noexcept : state_(std::move(state)) {}

    Ref<Detail::FutureState<T>> state_;
};

/// @brief Writer side of the Future / Promise pair. Holds the same shared state; the
///        producer calls Set exactly once. A Promise that is destroyed without Set being
///        called leaves any waiting Futures permanently unfulfilled — v1 has no
///        broken-promise detection, callers must arrange to Set even on failure (typically
///        Set(T{}) for the failure sentinel).
template<typename T>
class Promise
{
public:
    Promise() : state_(MakeRef<Detail::FutureState<T>>()) {}

    [[nodiscard]] Future<T> GetFuture() const { return Future<T>(state_); }

    void Set(T value) { state_->Set(std::move(value)); }

private:
    Ref<Detail::FutureState<T>> state_;
};

} // namespace Hylux
