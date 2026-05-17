/// @file
/// @brief RGContext implementation.

#include "RenderGraph/RGContext.h"

#include "RenderGraph/Internal/RGResourceRegistry.h"

#include <cassert>

namespace Hylux::RG
{

RGContext::RGContext(Internal::RGResourceRegistry* registry) noexcept
    : registry_(registry)
{
}

RHI::IRHITexture* RGContext::GetTexture(RGTextureHandle handle) const
{
    assert(handle.IsValid());
    return registry_->GetTexture(handle.Index());
}

RHI::IRHITextureView* RGContext::GetTextureView(RGTextureHandle handle,
                                                const RHI::TextureViewDesc& desc)
{
    assert(handle.IsValid());
    return registry_->GetTextureView(handle.Index(), desc);
}

RHI::IRHIBuffer* RGContext::GetBuffer(RGBufferHandle handle) const
{
    assert(handle.IsValid());
    return registry_->GetBuffer(handle.Index());
}

RHI::IRHIDevice* RGContext::GetDevice() const noexcept
{
    return registry_->GetDevice();
}

} // namespace Hylux::RG
