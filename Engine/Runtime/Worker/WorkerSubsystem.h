/// @file
/// @brief Engine subsystem wrapper for WorkerThreadPool. Owns the pool for the lifetime of
///        the Engine. Other subsystems list this in GetDependencies to grab a worker
///        executor in their own Initialize.

#pragma once

#include "Core/Async/WorkerThreadPool.h"
#include "Core/Reflection/TypeId.h"
#include "Engine/ISubsystem.h"

#include <memory>
#include <vector>

namespace Hylux
{

/// @brief Subsystem that brings up the engine-wide CPU worker pool. Construct via
///        Engine::RegisterSubsystem<WorkerSubsystem>(cfg) before Engine::Initialize.
class WorkerSubsystem : public ISubsystem
{
public:
    explicit WorkerSubsystem(WorkerThreadPool::Config cfg = {}) noexcept;
    ~WorkerSubsystem() override = default;

    WorkerSubsystem(const WorkerSubsystem&)            = delete;
    WorkerSubsystem& operator=(const WorkerSubsystem&) = delete;

    /// @brief Constructs and starts the pool.
    void Initialize(Engine& engine) override;

    /// @brief Stops and destroys the pool, joining all worker threads.
    void Shutdown() override;

    /// @brief No subsystem dependencies — the pool is a pure utility.
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override { return {}; }

    /// @brief Returns the executor backed by the pool. Stable for the subsystem's lifetime
    ///        once Initialize has run. Throws via assertion if accessed before Initialize.
    [[nodiscard]] WorkerExecutor& GetWorkerExecutor() noexcept;

    /// @brief Returns the underlying pool for callers that need worker-count / advanced
    ///        introspection (e.g. RenderSubsystem sizing per-worker secondary CL pools).
    [[nodiscard]] WorkerThreadPool& GetPool() noexcept;

    /// @brief Convenience: pool worker count. Valid after Initialize. Returns 0 before.
    [[nodiscard]] int GetWorkerCount() const noexcept;

private:
    WorkerThreadPool::Config          config_;
    std::unique_ptr<WorkerThreadPool> pool_;
};

} // namespace Hylux
