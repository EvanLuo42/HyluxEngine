/// @file
/// @brief Shared transient render targets that paths may import / reuse across their passes
///        (HZB, history color, motion buffer, etc.). Stage 1 placeholder — the pool is empty.

#pragma once

namespace Hylux::Renderer
{

/// @brief Per-view shared resource pool. Stage 1 is a placeholder; real allocators land
///        with the HZB / temporal history work in later stages.
class RenderResources
{
public:
    RenderResources() = default;
    ~RenderResources() = default;

    RenderResources(const RenderResources&) = delete;
    RenderResources& operator=(const RenderResources&) = delete;
};

} // namespace Hylux::Renderer
