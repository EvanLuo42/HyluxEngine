/// @file
/// @brief Setup-time builder handed to RGRasterPass::Setup. Adds color/depth attachment
///        declarations on top of RGBuilder's resource/dependency surface.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGHandle.h"

#include <cstdint>

namespace Hylux::RG
{

/// @brief Builder extension exposing raster-only API. Calling SetColorAttachment /
///        SetDepthAttachment implicitly records a WriteTexture on the underlying handle with
///        the appropriate layout, stage, and access masks, so the user does not duplicate the
///        dependency manually.
class RGRasterBuilder : public RGBuilder
{
public:
    /// @brief Binds a color attachment at the given slot. Internally records a WriteTexture
    ///        on the handle (ColorAttachment layout, ColorAttachmentOutput stage,
    ///        ColorAttachmentWrite access) and returns the resulting versioned handle so the
    ///        caller can keep reading or writing the rendered contents in later passes.
    ///        resolveHandle is optional; pass a default-constructed handle to indicate "no
    ///        MSAA resolve". When non-null, it is also Write-recorded and the returned handle
    ///        refers to the multisampled target — the resolve target version is consumed
    ///        internally by RenderingInfo.
    [[nodiscard]] RGTextureHandle SetColorAttachment(std::uint32_t   slot,
                                                     RGTextureHandle handle,
                                                     RHI::LoadOp     loadOp,
                                                     RHI::StoreOp    storeOp,
                                                     RHI::ClearValue clearValue   = {},
                                                     RGTextureHandle resolveHandle = {});

    /// @brief Binds the depth (or depth-stencil) attachment. Same Write semantics as
    ///        SetColorAttachment. There is exactly one per pass.
    [[nodiscard]] RGTextureHandle SetDepthAttachment(RGTextureHandle handle,
                                                     RHI::LoadOp     loadOp,
                                                     RHI::StoreOp    storeOp,
                                                     RHI::ClearValue clearValue = {});

    /// @brief Sets the render area for the pass's BeginRendering call. Defaults to the size
    ///        of the first color attachment if not specified.
    void SetRenderArea(RHI::Rect2D area);

    /// @brief Declares that this raster pass reads `handle` as an input attachment — i.e. it
    ///        samples (at the same fragment coordinate) data the previous pass wrote into a
    ///        color or depth attachment. Recorded as a ReadTexture with
    ///        AccessMask::InputAttachmentRead at the pixel-shader stage. This is the
    ///        precondition the future subpass-merger looks for when deciding whether two
    ///        consecutive raster passes can share one render-pass instance and keep the
    ///        attachment in tile memory. The current executor inserts a normal sampled-read
    ///        barrier; once the merger lands, it will rewrite the layout to a tile-local
    ///        read and elide the store/load round-trip.
    void ReadInputAttachment(RGTextureHandle handle);

private:
    friend class RenderGraph;
    RGRasterBuilder(RenderGraph* graph, std::uint32_t passIndex) noexcept
        : RGBuilder(graph, passIndex) {}
};

} // namespace Hylux::RG
