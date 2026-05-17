/// @file
/// @brief Access descriptors used when declaring read/write dependencies on RenderGraph resources.

#pragma once

#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"

namespace Hylux::RG
{

/// @brief Per-access description for a texture: target layout plus the sync2 stage and access
///        masks the consuming pass will use. Defaults model a shader-sampled read in a pixel shader.
struct RGTextureAccess
{
    RHI::ImageLayout       layout{RHI::ImageLayout::ShaderReadOnly};
    RHI::PipelineStageMask stages{RHI::PipelineStageMask::PixelShader};
    RHI::AccessMask        access{RHI::AccessMask::ShaderResourceRead};
};

/// @brief Per-access description for a buffer: sync2 stage and access masks. Defaults model a
///        structured buffer read in a compute shader.
struct RGBufferAccess
{
    RHI::PipelineStageMask stages{RHI::PipelineStageMask::ComputeShader};
    RHI::AccessMask        access{RHI::AccessMask::ShaderResourceRead};
};

} // namespace Hylux::RG
