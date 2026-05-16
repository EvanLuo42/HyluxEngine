/// @file
/// @brief Pipeline state object interfaces: graphics, compute, ray tracing.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIPipelineDesc.h"

namespace Hylux::RHI
{

/// @brief Compiled graphics pipeline.
class IRHIGraphicsPipeline : public IRHIObject
{
public:
    /// @brief Returns the descriptor the pipeline was created with.
    [[nodiscard]] virtual const GraphicsPipelineDesc& GetDesc() const noexcept = 0;
};

/// @brief Compiled compute pipeline.
class IRHIComputePipeline : public IRHIObject
{
public:
    /// @brief Returns the descriptor the pipeline was created with.
    [[nodiscard]] virtual const ComputePipelineDesc& GetDesc() const noexcept = 0;
};

/// @brief Compiled ray tracing pipeline.
class IRHIRayTracingPipeline : public IRHIObject
{
public:
    /// @brief Returns the descriptor the pipeline was created with.
    [[nodiscard]] virtual const RayTracingPipelineDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the shader group handle size in bytes for the backend.
    [[nodiscard]] virtual std::uint32_t GetShaderGroupHandleSize() const noexcept = 0;
};

} // namespace Hylux::RHI
