/// @file
/// @brief RGRasterBuilder implementation. Each attachment setter implicitly issues a
///        WriteTexture with the matching layout/stage/access masks.

#include "RenderGraph/RGRasterBuilder.h"

#include "RenderGraph/RenderGraph.h"

#include <cassert>

namespace Hylux::RG
{

namespace
{

constexpr RGTextureAccess kColorAttachmentAccess{
    RHI::ImageLayout::ColorAttachment,
    RHI::PipelineStageMask::ColorAttachmentOutput,
    RHI::AccessMask::ColorAttachmentWrite,
};

constexpr RGTextureAccess kDepthAttachmentAccess{
    RHI::ImageLayout::DepthStencilAttachment,
    RHI::PipelineStageMask::EarlyFragmentTests | RHI::PipelineStageMask::LateFragmentTests,
    RHI::AccessMask::DepthStencilAttachmentRead | RHI::AccessMask::DepthStencilAttachmentWrite,
};

constexpr RGTextureAccess kResolveAttachmentAccess{
    RHI::ImageLayout::ColorAttachment,
    RHI::PipelineStageMask::ColorAttachmentOutput,
    RHI::AccessMask::ColorAttachmentWrite,
};

constexpr RGTextureAccess kInputAttachmentAccess{
    RHI::ImageLayout::ShaderReadOnly,
    RHI::PipelineStageMask::PixelShader,
    RHI::AccessMask::InputAttachmentRead,
};

} // namespace

RGTextureHandle RGRasterBuilder::SetColorAttachment(std::uint32_t   slot,
                                                    RGTextureHandle handle,
                                                    RHI::LoadOp     loadOp,
                                                    RHI::StoreOp    storeOp,
                                                    RHI::ClearValue clearValue,
                                                    RGTextureHandle resolveHandle)
{
    assert(handle.IsValid() && "SetColorAttachment: invalid color handle");

    const RGTextureHandle written = graph_->RecordTextureWrite(passIndex_, handle, kColorAttachmentAccess);

    RGTextureHandle resolveWritten{};
    if (resolveHandle.IsValid())
    {
        resolveWritten = graph_->RecordTextureWrite(passIndex_, resolveHandle, kResolveAttachmentAccess);
    }

    graph_->RecordColorAttachment(passIndex_, slot, written, loadOp, storeOp, clearValue, resolveWritten);
    return written;
}

RGTextureHandle RGRasterBuilder::SetDepthAttachment(RGTextureHandle handle,
                                                    RHI::LoadOp     loadOp,
                                                    RHI::StoreOp    storeOp,
                                                    RHI::ClearValue clearValue)
{
    assert(handle.IsValid() && "SetDepthAttachment: invalid depth handle");

    const RGTextureHandle written = graph_->RecordTextureWrite(passIndex_, handle, kDepthAttachmentAccess);
    graph_->RecordDepthAttachment(passIndex_, written, loadOp, storeOp, clearValue);
    return written;
}

void RGRasterBuilder::SetRenderArea(RHI::Rect2D area)
{
    graph_->RecordRenderArea(passIndex_, area);
}

void RGRasterBuilder::ReadInputAttachment(RGTextureHandle handle)
{
    assert(handle.IsValid() && "ReadInputAttachment: invalid handle");
    graph_->RecordTextureRead(passIndex_, handle, kInputAttachmentAccess);
}

} // namespace Hylux::RG
