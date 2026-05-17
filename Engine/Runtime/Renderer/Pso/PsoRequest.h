/// @file
/// @brief Logical PSO request handed to PsoCache::GetOrCreate. Identifies a pipeline by
///        (passNameHash, permutationKey, optional materialAssetHash) and a pointer to the
///        long-lived PipelineRenderState plus the per-draw render-target formats / multiview.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHITypes.h"
#include "Renderer/Pso/PipelineRenderState.h"

#include <array>
#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Render-target format set + multiview mask. Hashed independently from
///        PipelineRenderState so PSO blob invalidation can scope to RT-format changes only.
struct PsoFormatKey
{
    std::array<RHI::Format, RHI::kMaxColorAttachments> colorFormats{};
    RHI::Format   depthStencilFormat{RHI::Format::Unknown};
    std::uint32_t sampleCount{1};
    std::uint32_t viewMask{0};
};

/// @brief Inputs PsoCache needs to resolve shaders, layouts, and the GraphicsPipelineDesc.
///        renderState must point at a stable PipelineRenderState (path-owned). When
///        materialAssetHash is zero the cache treats the request as a global pass shader
///        and consults ShaderSubsystem::GetPassShader; otherwise it goes through
///        MaterialShaderMap.
struct PsoRequest
{
    std::uint64_t              passNameHash{0};
    std::uint64_t              permutationKey{0};
    std::uint64_t              materialAssetHash{0};
    const PipelineRenderState* renderState{nullptr};
    PsoFormatKey               formats{};
};

} // namespace Hylux::Renderer
