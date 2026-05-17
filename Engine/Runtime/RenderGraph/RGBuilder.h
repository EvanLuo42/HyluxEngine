/// @file
/// @brief Setup-time builder handed to RGPass::Setup. Declares transient and imported resources
///        and records read/write dependencies that drive compilation.

#pragma once

#include "RHI/RHIForward.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGHandle.h"

#include <cstdint>
#include <string_view>

namespace Hylux::RG
{

class RenderGraph;

/// @brief Base builder API: resource declaration plus generic read/write/export. Used by
///        non-raster (compute, copy, blit) RGPass subclasses. RGRasterPass uses
///        RGRasterBuilder which extends this with attachment hooks.
class RGBuilder
{
public:
    RGBuilder(const RGBuilder&)            = delete;
    RGBuilder& operator=(const RGBuilder&) = delete;
    RGBuilder(RGBuilder&&)                 = delete;
    RGBuilder& operator=(RGBuilder&&)      = delete;

    /// @brief Declares a transient texture owned by the graph. The desc's usage flags are
    ///        augmented at Compile time with whichever flags the recorded accesses imply.
    RGTextureHandle CreateTexture(std::string_view name, const RHI::TextureDesc& desc);

    /// @brief Wraps an externally owned IRHITexture so it participates in graph scheduling.
    ///        initialLayout describes the texture's layout when Execute begins.
    RGTextureHandle ImportTexture(std::string_view name,
                                  RHI::IRHITexture* texture,
                                  RHI::ImageLayout  initialLayout);

    /// @brief Requests that the graph transition the texture into finalLayout after the last
    ///        recorded use. Without this call the imported texture is left in whatever layout
    ///        the last use produced. Typical use: ExportTexture(swapchain, ImageLayout::Present).
    void ExportTexture(RGTextureHandle handle, RHI::ImageLayout finalLayout);

    /// @brief Declares a transient buffer owned by the graph. Usage flags are augmented as
    ///        with CreateTexture.
    RGBufferHandle CreateBuffer(std::string_view name, const RHI::BufferDesc& desc);

    /// @brief Wraps an externally owned IRHIBuffer.
    RGBufferHandle ImportBuffer(std::string_view name, RHI::IRHIBuffer* buffer);

    /// @brief Records that the current pass reads the texture in the given layout/stage/access.
    ///        Establishes a graph edge from the producer of this handle's version to this pass.
    void ReadTexture(RGTextureHandle handle, RGTextureAccess access);

    /// @brief Records that the current pass writes the texture and returns the new SSA version.
    ///        The returned handle must be used for any further read/write of the new contents;
    ///        passing the old handle to a later builder call expresses a dependency on the
    ///        pre-write contents.
    [[nodiscard]] RGTextureHandle WriteTexture(RGTextureHandle handle, RGTextureAccess access);

    void ReadBuffer(RGBufferHandle handle, RGBufferAccess access);
    [[nodiscard]] RGBufferHandle WriteBuffer(RGBufferHandle handle, RGBufferAccess access);

    /// @brief Marks the current pass as a root of the live graph so it survives dead-pass
    ///        culling even if none of its outputs are consumed (present, debug overlays, etc.).
    void SetSideEffect();

protected:
    RGBuilder(RenderGraph* graph, std::uint32_t passIndex) noexcept
        : graph_(graph), passIndex_(passIndex) {}

    RenderGraph*  graph_{nullptr};
    std::uint32_t passIndex_{0};

private:
    friend class RenderGraph;
};

} // namespace Hylux::RG
