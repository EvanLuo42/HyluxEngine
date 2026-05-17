/// @file
/// @brief Render-thread internal view object built from a SceneViewRequest. Caches the
///        Math::Frustum derived from the request's view-projection matrix so DrawList
///        culling never recomputes it across passes.

#pragma once

#include "Core/Math/Frustum.h"
#include "Renderer/RendererForward.h"
#include "Renderer/View/SceneViewRequest.h"

namespace Hylux::Renderer
{

/// @brief Immutable copy of a SceneViewRequest projected onto the render thread. Holds
///        just enough state for the path to drive RenderGraph construction, plus a
///        precomputed frustum for culling.
class SceneView
{
public:
    explicit SceneView(const SceneViewRequest& request) noexcept;

    SceneView(const SceneView&) = default;
    SceneView& operator=(const SceneView&) = default;
    SceneView(SceneView&&) noexcept = default;
    SceneView& operator=(SceneView&&) noexcept = default;

    [[nodiscard]] IRenderPath*             GetPath() const noexcept { return request_.path; }
    [[nodiscard]] const SceneViewMatrices& GetMatrices() const noexcept { return request_.matrices; }
    [[nodiscard]] std::uint32_t            GetViewMask() const noexcept { return request_.viewMask; }
    [[nodiscard]] std::uint32_t            GetLayerMask() const noexcept { return request_.layerMask; }
    [[nodiscard]] RHI::IRHITexture*        GetExternalTarget() const noexcept { return request_.externalTarget; }
    [[nodiscard]] RHI::Format              GetTargetFormat() const noexcept { return request_.targetFormat; }
    [[nodiscard]] RHI::Rect2D              GetTargetRect() const noexcept { return request_.targetRect; }
    [[nodiscard]] void*                    GetUserPayload() const noexcept { return request_.userPayload; }

    /// @brief Frustum extracted once at construction from request.matrices.viewProjection.
    [[nodiscard]] const Math::Frustum& GetFrustum() const noexcept { return frustum_; }

private:
    SceneViewRequest request_;
    Math::Frustum    frustum_;
};

} // namespace Hylux::Renderer
