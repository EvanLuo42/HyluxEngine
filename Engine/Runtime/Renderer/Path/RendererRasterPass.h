/// @file
/// @brief RG::RGRasterPass bridge. Path authors derive from this and implement
///        SetupRenderer / ExecuteRenderer; the bridge methods are sealed and route to the
///        renderer's typed builder / context so users never see RG::RGRasterBuilder or
///        IRHICommandList directly (unless they reach for GetCommandList).

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGRasterPass.h"
#include "Renderer/Pso/PsoRequest.h"
#include "Renderer/RendererForward.h"

#include <cstdint>
#include <string_view>

namespace Hylux::RG
{
class RGRasterBuilder;
class RGContext;
} // namespace Hylux::RG

namespace Hylux::RHI
{
struct TextureDesc;
}

namespace Hylux::Renderer
{

class SceneView;
class RendererRasterPass;

/// @brief Thin authoring wrapper around RG::RGRasterBuilder. Carries a back-pointer to the
///        pass so format-key declarations can flow into the pass's stored PsoFormatKey
///        without polluting RG with renderer-specific state.
class RendererRasterBuilder
{
public:
    RendererRasterBuilder(RG::RGRasterBuilder& inner, RendererRasterPass& pass) noexcept : inner_(inner), pass_(pass) {}

    /// @brief Imports an externally owned IRHITexture (typically the view's external
    ///        target) so it participates in graph scheduling.
    [[nodiscard]] RG::RGTextureHandle ImportTexture(std::string_view name, RHI::IRHITexture* texture,
                                                    RHI::ImageLayout initialLayout) const;

    /// @brief Declares a transient texture owned by the graph for this frame.
    [[nodiscard]] RG::RGTextureHandle CreateTexture(std::string_view name, const RHI::TextureDesc& desc) const;

    /// @brief Binds a color attachment for this pass. Returns the resulting versioned handle.
    [[nodiscard]] RG::RGTextureHandle SetColorAttachment(std::uint32_t slot, RG::RGTextureHandle handle,
                                                         RHI::LoadOp loadOp, RHI::StoreOp storeOp,
                                                         RHI::ClearValue     clearValue = {},
                                                         RG::RGTextureHandle resolveHandle = {}) const;

    /// @brief Binds the (single) depth attachment for this pass.
    [[nodiscard]] RG::RGTextureHandle SetDepthAttachment(RG::RGTextureHandle handle, RHI::LoadOp loadOp,
                                                         RHI::StoreOp storeOp, RHI::ClearValue clearValue = {}) const;

    /// @brief Transitions an imported texture into finalLayout after its last use.
    void ExportTexture(RG::RGTextureHandle handle, RHI::ImageLayout finalLayout) const;

    /// @brief Sets the BeginRendering render area. Defaults to the first color attachment's size.
    void SetRenderArea(RHI::Rect2D area) const;

    /// @brief Marks the pass as a graph root so dead-pass culling never drops it.
    void SetSideEffect() const;

    /// @brief Records the render-target format set the pass commits to. PsoCache uses this
    ///        when resolving pipelines for DrawRendererList; missing it means the cache key
    ///        defaults to Format::Unknown across the board, which is almost certainly wrong.
    void SetRenderingFormats(const PsoFormatKey& key) const;

private:
    RG::RGRasterBuilder& inner_;
    RendererRasterPass&  pass_;
};

/// @brief Base for raster passes authored through the Renderer module. Subclasses implement
///        SetupRenderer + ExecuteRenderer; the RG bridges are sealed.
class RendererRasterPass : public RG::RGRasterPass
{
public:
    /// @brief Sentinel returned by GetTransformBindlessIndex when no published half exists.
    static constexpr std::uint32_t kInvalidBindlessIndex = 0xFFFFFFFFu;

    /// @brief Attaches the per-frame execution-time state. Path code calls this immediately
    ///        after AddPass<TPass>(); RG runs Setup synchronously, but Execute runs later
    ///        and needs the view + PsoCache + current transform-buffer half.
    void SetExecutionContext(const SceneView* view, PsoCache* psoCache, std::uint32_t transformBindlessIndex) noexcept
    {
        activeView_ = view;
        psoCache_ = psoCache;
        transformBindlessIndex_ = transformBindlessIndex;
    }

    /// @brief Used by RendererRasterBuilder::SetRenderingFormats. Path code may also call
    ///        directly from its SetupRenderer body if it prefers.
    void SetFormats(const PsoFormatKey& key) noexcept { formats_ = key; }

    [[nodiscard]] const PsoFormatKey& GetFormats() const noexcept { return formats_; }

    /// @brief Sealed bridge. Forwards to SetupRenderer with a Renderer-typed builder.
    void Setup(RG::RGRasterBuilder& builder) final;

    /// @brief Sealed bridge. Forwards to ExecuteRenderer with a Renderer-typed context.
    void Execute(RG::RGContext& ctx, RHI::IRHICommandList& cmd) final;

protected:
    /// @brief Declare attachments, transients, and read/write dependencies here.
    virtual void SetupRenderer(RendererRasterBuilder& builder) = 0;

    /// @brief Record draws (via RendererPassContext::DrawRendererList) here. Empty bodies
    ///        are valid for passes that only rely on LoadOp::Clear / resolve side effects.
    virtual void ExecuteRenderer(RendererPassContext& ctx) = 0;

private:
    const SceneView* activeView_{nullptr};
    PsoCache*        psoCache_{nullptr};
    std::uint32_t    transformBindlessIndex_{kInvalidBindlessIndex};
    PsoFormatKey     formats_{};
};

} // namespace Hylux::Renderer
