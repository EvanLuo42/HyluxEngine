/// @file
/// @brief EngineLoop implementation: spawns the game thread, drives Initialize / Tick /
///        Shutdown on it, and drains submitted commands between ticks.

#include "Engine/EngineLoop.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Engine/Engine.h"

#include <chrono>
#include <exception>

namespace Hylux
{

EngineLoop::EngineLoop(Engine& engine, Config cfg) : engine_(engine), config_(std::move(cfg)) {}

EngineLoop::~EngineLoop()
{
    Stop();
}

Future<bool> EngineLoop::Start()
{
    if (running_.load(std::memory_order_acquire))
    {
        return initPromise_.GetFuture();
    }
    running_.store(true, std::memory_order_release);
    stop_.store(false, std::memory_order_release);
    Future<bool> fut = initPromise_.GetFuture();
    thread_ = std::thread([this] { ThreadEntry(); });
    return fut;
}

void EngineLoop::Stop()
{
    stop_.store(true, std::memory_order_release);
    wakeCv_.notify_all();
    if (thread_.joinable())
    {
        thread_.join();
    }
    running_.store(false, std::memory_order_release);
}

void EngineLoop::SubmitCommand(std::function<void(Engine&)> cmd)
{
    if (!cmd)
    {
        return;
    }
    {
        std::lock_guard lock(queueMu_);
        pending_.push_back(std::move(cmd));
    }
    wakeCv_.notify_all();
}

std::size_t EngineLoop::PendingCommandCount() const noexcept
{
    std::lock_guard lock(queueMu_);
    return pending_.size();
}

void EngineLoop::DrainCommands()
{
    std::vector<std::function<void(Engine&)>> local;
    {
        std::lock_guard lock(queueMu_);
        local.swap(pending_);
    }
    for (auto& cmd : local)
    {
        cmd(engine_);
    }
}

void EngineLoop::ThreadEntry()
{
    bool initOk = false;
    try
    {
        engine_.Initialize();
        initOk = true;
    }
    catch (const std::exception& e)
    {
        HYLUX_LOG_FATAL(LogEngine, "EngineLoop: Engine::Initialize threw: {}", e.what());
    }
    catch (...)
    {
        HYLUX_LOG_FATAL(LogEngine, "EngineLoop: Engine::Initialize threw unknown exception");
    }
    initPromise_.Set(initOk);
    if (!initOk)
    {
        running_.store(false, std::memory_order_release);
        return;
    }

    using clock = std::chrono::steady_clock;
    const auto period = config_.targetHz > 0.0f
                            ? std::chrono::duration_cast<clock::duration>(
                                  std::chrono::duration<float>(1.0f / config_.targetHz))
                            : clock::duration::zero();

    auto last = clock::now();
    while (!stop_.load(std::memory_order_acquire))
    {
        DrainCommands();

        const auto  now = clock::now();
        const float dt  = std::chrono::duration<float>(now - last).count();
        last            = now;
        try
        {
            engine_.Tick(dt);
        }
        catch (const std::exception& e)
        {
            HYLUX_LOG_ERROR(LogEngine, "EngineLoop: Engine::Tick threw: {}", e.what());
        }
        catch (...)
        {
            HYLUX_LOG_ERROR(LogEngine, "EngineLoop: Engine::Tick threw unknown exception");
        }

        if (period.count() > 0)
        {
            const auto deadline = now + period;
            std::unique_lock lock(queueMu_);
            wakeCv_.wait_until(lock, deadline, [this] {
                return stop_.load(std::memory_order_acquire) || !pending_.empty();
            });
        }
    }

    try
    {
        engine_.Shutdown();
    }
    catch (const std::exception& e)
    {
        HYLUX_LOG_ERROR(LogEngine, "EngineLoop: Engine::Shutdown threw: {}", e.what());
    }
    catch (...)
    {
        HYLUX_LOG_ERROR(LogEngine, "EngineLoop: Engine::Shutdown threw unknown exception");
    }
}

} // namespace Hylux
