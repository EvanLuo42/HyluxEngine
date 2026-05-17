/// @file
/// @brief Game-thread input that requests one rendered view this frame. The host attaches
///        the IRenderPath* to use; the renderer does not classify views by type — the path
///        identity is the classification.

#pragma once

#include "Core/Math/Mat4.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"
#include "Renderer/RendererForward.h"

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Pre-baked transforms describing a camera. World matrices live elsewhere; these
///        are evaluated game-side from a camera component (when an ECS lands) or written
///        directly by the host (editor viewport).
struct SceneViewMatrices
{
    Math::Mat4 view;
    Math::Mat4 projection;
    Math::Mat4 viewProjection;
    Math::Mat4 invViewProjection;
};

/// @brief Game-thread description of one view to render this frame. The renderer copies
///        the contents into an internal SceneView before crossing into the render thread.
struct SceneViewRequest
{
    /// @brief Required. Non-owning pointer to a path previously handed to RegisterRenderPath.
    IRenderPath* path{nullptr};

    SceneViewMatrices matrices{};

    /// @brief Multiview mask. 0 == single view; non-zero engages the multiview PSO path.
    std::uint32_t viewMask{0};

    /// @brief Visibility bitfield matched against PrimitiveProxy::layerMask. 0xFFFFFFFF
    ///        means "render every layer". Stage 1 ignores this until DrawList lands.
    std::uint32_t layerMask{0xFFFFFFFFu};

    /// @brief Optional render target. For the editor this is a Qt-side IRHITexture wrapped
    ///        as an IRHITexture; for the game launcher it is the current swapchain back
    ///        buffer. nullptr means "no display target" (debug / off-screen only).
    RHI::IRHITexture* externalTarget{nullptr};
    RHI::Format       targetFormat{RHI::Format::Unknown};
    RHI::Rect2D       targetRect{};

    /// @brief Path-specific extension payload. The renderer treats it as opaque; the path
    ///        casts back to its own struct.
    void* userPayload{nullptr};
};

} // namespace Hylux::Renderer
