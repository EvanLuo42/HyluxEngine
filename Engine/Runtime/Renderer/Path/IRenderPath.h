/// @file
/// @brief Per-view graph weaver. Implementations populate the RenderGraph for one
///        SceneView through RenderContext. They must not touch IRHICommandList directly —
///        the only authoring surface is DrawList + RendererPassContext.

#pragma once

#include "Renderer/RendererForward.h"
#include "RHI/RHIForward.h"

#include <string_view>

namespace Hylux::Renderer
{

/// @brief Interface every render path implements. The renderer owns instances handed to it
///        via RenderSubsystem::RegisterRenderPath and dispatches BuildGraph per view per
///        frame. Initialize and Shutdown happen once on the render thread.
class IRenderPath
{
public:
    virtual ~IRenderPath() = default;

    /// @brief Short identifier shown in stats, debug overlays, and capture markers.
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;

    /// @brief Allocates persistent resources (history buffers, look-up textures) the first
    ///        time the path is used. Called on the render thread.
    virtual void Initialize(RHI::IRHIDevice& device) = 0;

    /// @brief Releases anything Initialize allocated. Called on the render thread.
    virtual void Shutdown() = 0;

    /// @brief Adds passes to the per-frame RenderGraph for one SceneView. Called on the
    ///        render thread between StructuralCommandQueue drain and graph compilation.
    virtual void BuildGraph(RenderContext& ctx) = 0;
};

} // namespace Hylux::Renderer
