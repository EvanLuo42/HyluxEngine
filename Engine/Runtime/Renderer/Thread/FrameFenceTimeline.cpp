/// @file
/// @brief FrameFenceTimeline implementation.

#include "Renderer/Thread/FrameFenceTimeline.h"

namespace Hylux::Renderer
{

FrameFenceTimeline::FrameFenceTimeline(std::uint32_t framesInFlight) noexcept
    : framesInFlight_(framesInFlight == 0 ? 1u : framesInFlight),
      semaphore_(static_cast<std::ptrdiff_t>(framesInFlight == 0 ? 1u : framesInFlight))
{
}

std::uint64_t FrameFenceTimeline::AcquireGameFrameId()
{
    semaphore_.acquire();
    return nextFrameId_.fetch_add(1, std::memory_order_acq_rel) + 1ull;
}

void FrameFenceTimeline::ReleaseRenderFrame() noexcept
{
    semaphore_.release();
}

void FrameFenceTimeline::WaitIdle()
{
    for (std::uint32_t i = 0; i < framesInFlight_; ++i)
    {
        semaphore_.acquire();
    }
    for (std::uint32_t i = 0; i < framesInFlight_; ++i)
    {
        semaphore_.release();
    }
}

} // namespace Hylux::Renderer
