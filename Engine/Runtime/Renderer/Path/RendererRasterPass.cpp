/// @file
/// @brief RendererRasterBuilder + RendererRasterPass implementation.

#include "Renderer/Path/RendererRasterPass.h"

#include "RHI/RHIResourceDesc.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGContext.h"
#include "RenderGraph/RGRasterBuilder.h"
#include "Renderer/Path/RendererPassContext.h"
#include "Renderer/View/SceneView.h"

#include <cassert>

namespace Hylux::Renderer
{

RG::RGTextureHandle RendererRasterBuilder::ImportTexture(std::string_view name, RHI::IRHITexture* texture,
                                                         RHI::ImageLayout initialLayout) const
{
    return inner_.ImportTexture(name, texture, initialLayout);
}

RG::RGTextureHandle RendererRasterBuilder::CreateTexture(std::string_view name, const RHI::TextureDesc& desc) const
{
    return inner_.CreateTexture(name, desc);
}

RG::RGTextureHandle RendererRasterBuilder::SetColorAttachment(std::uint32_t slot, RG::RGTextureHandle handle,
                                                              RHI::LoadOp loadOp, RHI::StoreOp storeOp,
                                                              RHI::ClearValue     clearValue,
                                                              RG::RGTextureHandle resolveHandle) const
{
    return inner_.SetColorAttachment(slot, handle, loadOp, storeOp, clearValue, resolveHandle);
}

RG::RGTextureHandle RendererRasterBuilder::SetDepthAttachment(RG::RGTextureHandle handle, RHI::LoadOp loadOp,
                                                              RHI::StoreOp storeOp, RHI::ClearValue clearValue) const
{
    return inner_.SetDepthAttachment(handle, loadOp, storeOp, clearValue);
}

void RendererRasterBuilder::ExportTexture(RG::RGTextureHandle handle, RHI::ImageLayout finalLayout) const
{
    inner_.ExportTexture(handle, finalLayout);
}

void RendererRasterBuilder::SetRenderArea(RHI::Rect2D area) const
{
    inner_.SetRenderArea(area);
}

void RendererRasterBuilder::SetSideEffect() const
{
    inner_.SetSideEffect();
}

void RendererRasterBuilder::SetRenderingFormats(const PsoFormatKey& key) const
{
    pass_.SetFormats(key);
}

void RendererRasterPass::Setup(RG::RGRasterBuilder& builder)
{
    RendererRasterBuilder wrapped(builder, *this);
    SetupRenderer(wrapped);
}

void RendererRasterPass::Execute(RG::RGContext& ctx, RHI::IRHICommandList& cmd)
{
    assert(activeView_ != nullptr && "RendererRasterPass::SetExecutionContext must be called before Execute");
    RendererPassContext passCtx(ctx, cmd, *activeView_, psoCache_, transformBindlessIndex_, formats_);
    ExecuteRenderer(passCtx);
}

} // namespace Hylux::Renderer
