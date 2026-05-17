/// @file
/// @brief Execute-time accessor handed to RGPass::Execute. Resolves handles to realized RHI
///        resources and caches IRHITextureView instances.

#pragma once

#include "RHI/RHIForward.h"
#include "RenderGraph/RGHandle.h"

namespace Hylux::RG
{

namespace Internal { class RGResourceRegistry; }

class RenderGraph;

/// @brief Read-only view onto realized graph resources, passed to RGPass::Execute. The same
///        instance is reused across all passes within a single Execute call.
class RGContext
{
public:
    /// @brief Returns the realized texture for the given handle. The version on the handle is
    ///        ignored at Execute time; the underlying RHI texture is identical across versions.
    [[nodiscard]] RHI::IRHITexture* GetTexture(RGTextureHandle handle) const;

    /// @brief Returns a cached IRHITextureView matching the request, creating one on miss.
    [[nodiscard]] RHI::IRHITextureView* GetTextureView(RGTextureHandle handle,
                                                       const RHI::TextureViewDesc& desc);

    /// @brief Returns the realized buffer for the given handle.
    [[nodiscard]] RHI::IRHIBuffer* GetBuffer(RGBufferHandle handle) const;

    /// @brief Returns the device the graph was constructed against. Useful for ad-hoc resource
    ///        creation inside Execute (e.g. uniform buffer uploads).
    [[nodiscard]] RHI::IRHIDevice* GetDevice() const noexcept;

private:
    friend class RenderGraph;
    explicit RGContext(Internal::RGResourceRegistry* registry) noexcept;

    Internal::RGResourceRegistry* registry_{nullptr};
};

} // namespace Hylux::RG
