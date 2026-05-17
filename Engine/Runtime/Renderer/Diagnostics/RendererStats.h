/// @file
/// @brief Per-frame statistics published by the RenderSubsystem.

#pragma once

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Snapshot of work performed during the most recently completed frame. Updated on
///        the render-side execution path; readable from the game thread after EndFrame.
struct RendererStats
{
    std::uint64_t frameId{0};
    std::uint32_t viewCount{0};
    std::uint32_t passCount{0};
    std::uint32_t drawCallCount{0};
    std::uint32_t psoMissCount{0};
    std::uint32_t psoHitCount{0};
    float         cpuFrameMs{0.0f};
    float         gpuFrameMs{0.0f};
};

} // namespace Hylux::Renderer
