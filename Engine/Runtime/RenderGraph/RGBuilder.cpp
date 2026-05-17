/// @file
/// @brief RGBuilder implementation. All entry points forward to the owning RenderGraph's
///        internal record* helpers, which manipulate per-pass and per-resource state.

#include "RenderGraph/RGBuilder.h"

#include "RenderGraph/RenderGraph.h"

#include <cassert>

namespace Hylux::RG
{

RGTextureHandle RGBuilder::CreateTexture(std::string_view name, const RHI::TextureDesc& desc)
{
    const std::uint32_t index = graph_->CreateTextureNode(name, desc);
    return RGTextureHandle(index, 0);
}

RGTextureHandle RGBuilder::ImportTexture(std::string_view name,
                                         RHI::IRHITexture* texture,
                                         RHI::ImageLayout  initialLayout)
{
    assert(texture != nullptr && "ImportTexture: texture must be non-null");
    const std::uint32_t index = graph_->ImportTextureNode(name, texture, initialLayout);
    return RGTextureHandle(index, 0);
}

void RGBuilder::ExportTexture(RGTextureHandle handle, RHI::ImageLayout finalLayout)
{
    assert(handle.IsValid() && "ExportTexture: invalid handle");
    graph_->SetTextureFinalLayout(handle.Index(), finalLayout);
}

RGBufferHandle RGBuilder::CreateBuffer(std::string_view name, const RHI::BufferDesc& desc)
{
    const std::uint32_t index = graph_->CreateBufferNode(name, desc);
    return RGBufferHandle(index, 0);
}

RGBufferHandle RGBuilder::ImportBuffer(std::string_view name, RHI::IRHIBuffer* buffer)
{
    assert(buffer != nullptr && "ImportBuffer: buffer must be non-null");
    const std::uint32_t index = graph_->ImportBufferNode(name, buffer);
    return RGBufferHandle(index, 0);
}

void RGBuilder::ReadTexture(RGTextureHandle handle, RGTextureAccess access)
{
    assert(handle.IsValid() && "ReadTexture: invalid handle");
    graph_->RecordTextureRead(passIndex_, handle, access);
}

RGTextureHandle RGBuilder::WriteTexture(RGTextureHandle handle, RGTextureAccess access)
{
    assert(handle.IsValid() && "WriteTexture: invalid handle");
    return graph_->RecordTextureWrite(passIndex_, handle, access);
}

void RGBuilder::ReadBuffer(RGBufferHandle handle, RGBufferAccess access)
{
    assert(handle.IsValid() && "ReadBuffer: invalid handle");
    graph_->RecordBufferRead(passIndex_, handle, access);
}

RGBufferHandle RGBuilder::WriteBuffer(RGBufferHandle handle, RGBufferAccess access)
{
    assert(handle.IsValid() && "WriteBuffer: invalid handle");
    return graph_->RecordBufferWrite(passIndex_, handle, access);
}

void RGBuilder::SetSideEffect()
{
    graph_->MarkPassSideEffect(passIndex_);
}

} // namespace Hylux::RG
