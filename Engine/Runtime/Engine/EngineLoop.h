/// @file
/// @brief Thread-driven runner for an Engine instance. Owns a game thread that calls
///        Engine::Initialize / Tick / Shutdown so the host (editor, launcher, headless tool)
///        never has to spin the loop itself. Exposes a thread-safe SubmitCommand entry point
///        so any thread can mutate engine state through closures executed between ticks.

#pragma once

#include "Core/Async/Future.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux
{

class Engine;

/// @brief Owns the std::thread that drives an Engine. Initialize / Tick / Shutdown all run
///        on that thread; SubmitCommand is the only thread-safe entry point for outside
///        callers. The Engine itself stays single-threaded — the loop just decides which
///        thread that is.
class EngineLoop
{
public:
    /// @brief Tunable parameters for the loop. Defaults target a 60 Hz fixed-step cadence.
    struct Config
    {
        float targetHz = 60.0f;
    };

    explicit EngineLoop(Engine& engine, Config cfg = {});
    ~EngineLoop();

    EngineLoop(const EngineLoop&)            = delete;
    EngineLoop& operator=(const EngineLoop&) = delete;
    EngineLoop(EngineLoop&&)                 = delete;
    EngineLoop& operator=(EngineLoop&&)      = delete;

    /// @brief Spawns the game thread and returns immediately. The future resolves to true
    ///        when Engine::Initialize has completed on the loop thread, or false if it
    ///        threw. Callers should chain via MainThreadDispatcher to land the continuation
    ///        back on the UI thread.
    [[nodiscard]] Future<bool> Start();

    /// @brief Signals the loop to exit at the next iteration boundary and joins the thread.
    ///        Engine::Shutdown runs on the loop thread before the join completes. Safe to
    ///        call multiple times; subsequent calls are no-ops.
    void Stop();

    /// @brief Thread-safe. The closure is queued and executed on the game thread between
    ///        the next drain and the next tick. Closures are executed in submission order.
    void SubmitCommand(std::function<void(Engine&)> cmd);

    /// @brief Convenience: submits a closure whose return value resolves a Future. The
    ///        closure must return a non-void value; use SubmitCommand for fire-and-forget.
    template<class F> auto Invoke(F&& fn);

    [[nodiscard]] bool        IsRunning() const noexcept { return running_.load(std::memory_order_acquire); }
    [[nodiscard]] std::size_t PendingCommandCount() const noexcept;

private:
    void ThreadEntry();
    void DrainCommands();

    Engine&  engine_;
    Config   config_;

    std::thread       thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};

    mutable std::mutex                            queueMu_;
    std::condition_variable                       wakeCv_;
    std::vector<std::function<void(Engine&)>>     pending_;

    Promise<bool> initPromise_;
};

template<class F> auto EngineLoop::Invoke(F&& fn)
{
    using R = std::invoke_result_t<F, Engine&>;
    static_assert(!std::is_void_v<R>, "EngineLoop::Invoke requires a non-void return; use SubmitCommand for void");
    Promise<R> promise;
    Future<R>  fut = promise.GetFuture();
    SubmitCommand([fn = std::forward<F>(fn), promise = std::move(promise)](Engine& engine) mutable {
        promise.Set(fn(engine));
    });
    return fut;
}

} // namespace Hylux
