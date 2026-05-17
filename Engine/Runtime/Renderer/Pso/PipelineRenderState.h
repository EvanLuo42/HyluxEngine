/// @file
/// @brief Immutable, often-shared bundle covering every graphics pipeline knob that is not
///        a shader, a layout, or a render-target format. Path code holds long-lived
///        PipelineRenderState instances and feeds pointers to them into PsoRequest, so the
///        same bundle can drive many draws without per-draw re-construction.

#pragma once

#include "RHI/RHIPipelineDesc.h"

namespace Hylux::Renderer
{

/// @brief Aggregate of state baked into a graphics PSO that the renderer expects to remain
///        constant across many requests. Lifetime: typically static / owned by a path.
struct PipelineRenderState
{
    RHI::VertexInputDesc   vertexInput{};
    RHI::PrimitiveTopology topology{RHI::PrimitiveTopology::TriangleList};
    RHI::RasterizerDesc    rasterizer{};
    RHI::DepthStencilDesc  depthStencil{};
    RHI::BlendDesc         blend{};
};

} // namespace Hylux::Renderer
