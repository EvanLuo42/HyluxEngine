/// @file
/// @brief SceneView implementation.

#include "Renderer/View/SceneView.h"

namespace Hylux::Renderer
{

SceneView::SceneView(const SceneViewRequest& request) noexcept
    : request_(request), frustum_(Math::Frustum::FromViewProj(request.matrices.viewProjection))
{}

} // namespace Hylux::Renderer
