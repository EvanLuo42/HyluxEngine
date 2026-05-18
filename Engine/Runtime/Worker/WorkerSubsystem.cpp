/// @file
/// @brief WorkerSubsystem implementation. Pool comes up in Initialize and tears down in
///        Shutdown; before Initialize the accessors trip a debug assert.

#include "Worker/WorkerSubsystem.h"

#include "Core/Utils/Assert.h"

namespace Hylux
{

WorkerSubsystem::WorkerSubsystem(WorkerThreadPool::Config cfg) noexcept : config_(cfg) {}

void WorkerSubsystem::Initialize(Engine& /*engine*/)
{
    pool_ = std::make_unique<WorkerThreadPool>(config_);
}

void WorkerSubsystem::Shutdown()
{
    pool_.reset();
}

WorkerExecutor& WorkerSubsystem::GetWorkerExecutor() noexcept
{
    HYLUX_ASSERT_MSG(pool_ != nullptr, "WorkerSubsystem::GetWorkerExecutor called before Initialize");
    return pool_->GetExecutor();
}

WorkerThreadPool& WorkerSubsystem::GetPool() noexcept
{
    HYLUX_ASSERT_MSG(pool_ != nullptr, "WorkerSubsystem::GetPool called before Initialize");
    return *pool_;
}

int WorkerSubsystem::GetWorkerCount() const noexcept
{
    return pool_ ? pool_->GetWorkerCount() : 0;
}

} // namespace Hylux
