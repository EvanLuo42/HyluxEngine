/// @file
/// @brief StructuralCommandQueue implementation.

#include "Renderer/Thread/StructuralCommandQueue.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace Hylux::Renderer
{
namespace
{

[[nodiscard]] bool ContainsBeginFrame(const std::deque<StructuralCommand>& queue) noexcept
{
    return std::ranges::any_of(queue,
                               [](const StructuralCommand& cmd) { return std::holds_alternative<BeginFrameCmd>(cmd); });
}

} // namespace

void StructuralCommandQueue::Enqueue(StructuralCommand command)
{
    {
        std::lock_guard lock(mutex_);
        if (stopped_)
        {
            return;
        }
        queue_.push_back(std::move(command));
    }
    cv_.notify_one();
}

std::vector<StructuralCommand> StructuralCommandQueue::DrainToFrameBoundary()
{
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return stopped_ || ContainsBeginFrame(queue_); });

    if (!ContainsBeginFrame(queue_))
    {
        return {};
    }

    const auto frameIt = std::ranges::find_if(
        queue_, [](const StructuralCommand& cmd) { return std::holds_alternative<BeginFrameCmd>(cmd); });
    const std::size_t count = static_cast<std::size_t>(std::distance(queue_.begin(), frameIt)) + 1;

    std::vector<StructuralCommand> drained;
    drained.reserve(count);
    std::move(queue_.begin(), frameIt + 1, std::back_inserter(drained));
    queue_.erase(queue_.begin(), frameIt + 1);
    return drained;
}

std::vector<StructuralCommand> StructuralCommandQueue::TryDrainAll()
{
    std::lock_guard lock(mutex_);
    if (queue_.empty())
    {
        return {};
    }
    std::vector<StructuralCommand> drained;
    drained.reserve(queue_.size());
    std::move(queue_.begin(), queue_.end(), std::back_inserter(drained));
    queue_.clear();
    return drained;
}

void StructuralCommandQueue::Stop()
{
    {
        std::lock_guard lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_all();
}

bool StructuralCommandQueue::IsStopped() const noexcept
{
    std::lock_guard lock(mutex_);
    return stopped_;
}

std::size_t StructuralCommandQueue::PendingCount() const noexcept
{
    std::lock_guard lock(mutex_);
    return queue_.size();
}

} // namespace Hylux::Renderer
