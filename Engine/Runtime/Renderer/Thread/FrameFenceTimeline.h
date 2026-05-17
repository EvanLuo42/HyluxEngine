/// @file
/// @brief Per-frame slot fence shared between the game and render threads. Caps the number
///        of frames the game thread can be ahead of the render thread to framesInFlight,
///        and exposes a monotonic frame id counter so commands can reference their owning
///        frame.

#pragma once

#include <atomic>
#include <cstdint>
#include <semaphore>

namespace Hylux::Renderer
{

/// @brief Counting-semaphore-backed frame slot manager. Game thread acquires a slot before
///        publishing a BeginFrame command; render thread releases the slot after the
///        frame's GPU work has retired.
class FrameFenceTimeline
{
public:
    explicit FrameFenceTimeline(std::uint32_t framesInFlight) noexcept;
    ~FrameFenceTimeline() = default;

    FrameFenceTimeline(const FrameFenceTimeline&) = delete;
    FrameFenceTimeline& operator=(const FrameFenceTimeline&) = delete;
    FrameFenceTimeline(FrameFenceTimeline&&) = delete;
    FrameFenceTimeline& operator=(FrameFenceTimeline&&) = delete;

    /// @brief Game thread: blocks until a slot is available, returns a fresh monotonic
    ///        frame id (1-based, never reused).
    [[nodiscard]] std::uint64_t AcquireGameFrameId();

    /// @brief Render thread: returns a slot after the GPU work is retired. Must be called
    ///        exactly once per AcquireGameFrameId.
    void ReleaseRenderFrame() noexcept;

    /// @brief Game thread: blocks until every in-flight slot has been released. The
    ///        method drains and then restores all permits so subsequent
    ///        AcquireGameFrameId calls behave unchanged.
    void WaitIdle();

    [[nodiscard]] std::uint32_t GetFramesInFlight() const noexcept { return framesInFlight_; }
    [[nodiscard]] std::uint64_t GetLastAcquiredFrameId() const noexcept { return nextFrameId_.load(); }

private:
    std::uint32_t              framesInFlight_;
    std::counting_semaphore<>  semaphore_;
    std::atomic<std::uint64_t> nextFrameId_{0};
};

} // namespace Hylux::Renderer
