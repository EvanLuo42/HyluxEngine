#pragma once

#include "Renderer/Diagnostics/RendererStats.h"

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Bookkeeping handed off between BeginFrame, SubmitFrame, and EndFrame.
class RendererFrame
{
public:
    RendererFrame() = default;

    [[nodiscard]] std::uint64_t GetFrameId() const noexcept { return frameId_; }
    [[nodiscard]] const RendererStats& GetStats() const noexcept { return stats_; }
    [[nodiscard]] RendererStats& GetMutableStats() noexcept { return stats_; }

    void AdvanceFrameId() noexcept
    {
        ++frameId_;
        stats_.frameId = frameId_;
    }

private:
    std::uint64_t frameId_{0};
    RendererStats stats_{};
};

} // namespace Hylux::Renderer
