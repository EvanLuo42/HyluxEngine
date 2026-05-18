/// @file
/// @brief Ray-tracing RenderGraph pass. Wraps DispatchRays and acceleration-structure builds
///        recorded inside Execute. Uses the graphics queue by default since AS-build +
///        DispatchRays typically share resources with raster passes that produced their
///        per-frame inputs.

#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "RenderGraph/RGPass.h"

namespace Hylux::RG
{

/// @brief Specialization of RGPass for ray-tracing workloads. The base Setup(RGBuilder&)
///        surface covers everything needed: declare reads on the per-frame TLAS / instance
///        buffer and any sampled textures, and writes on the output UAV. Execute issues
///        BuildAccelerationStructures (if rebuilding) then DispatchRays.
class RGRayTracingPass : public RGPass
{
    HYLUX_REFLECT_BODY(RGRayTracingPass)
public:
    [[nodiscard]] QueueAffinity GetQueueAffinity() const noexcept override
    {
        return QueueAffinity::Graphics;
    }
};

} // namespace Hylux::RG
