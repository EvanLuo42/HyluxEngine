/// @file
/// @brief Internal resource registry: maps RenderGraph handle indices to realized RHI resources
///        and caches IRHITextureView instances keyed by (textureIndex, view desc).

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/Internal/RGNode.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Hylux::RHI
{
class IRHIDevice;
class IRHITextureView;
} // namespace Hylux::RHI

namespace Hylux::RG::Internal
{

/// @brief Owns the per-frame view cache and resolves handle indices into realized RHI resources
///        by indexing into the RenderGraph's texture and buffer node arrays.
class RGResourceRegistry
{
public:
    RGResourceRegistry(RHI::IRHIDevice* device,
                       std::vector<RGTextureNode>* textures,
                       std::vector<RGBufferNode>* buffers) noexcept;

    [[nodiscard]] RHI::IRHIDevice* GetDevice() const noexcept { return device_; }

    /// @brief Returns the realized texture for the given node index, or null if not yet compiled.
    [[nodiscard]] RHI::IRHITexture* GetTexture(std::uint32_t textureIndex) const;

    /// @brief Returns the realized buffer for the given node index, or null if not yet compiled.
    [[nodiscard]] RHI::IRHIBuffer* GetBuffer(std::uint32_t bufferIndex) const;

    /// @brief Returns a cached IRHITextureView matching the request, creating one on miss.
    [[nodiscard]] RHI::IRHITextureView* GetTextureView(std::uint32_t textureIndex,
                                                       const RHI::TextureViewDesc& desc);

    /// @brief Releases all cached views. Called by RenderGraph between Execute and destruction.
    void Reset() noexcept;

private:
    struct ViewKey
    {
        std::uint32_t          textureIndex;
        RHI::TextureDimension  dimension;
        RHI::Format            format;
        RHI::SubresourceRange  range;
        std::uint8_t           viewFlags;
    };

    struct ViewKeyHash
    {
        std::size_t operator()(const ViewKey& key) const noexcept;
    };

    struct ViewKeyEq
    {
        bool operator()(const ViewKey& a, const ViewKey& b) const noexcept;
    };

    RHI::IRHIDevice*                                                                 device_{nullptr};
    std::vector<RGTextureNode>*                                                      textures_{nullptr};
    std::vector<RGBufferNode>*                                                       buffers_{nullptr};
    mutable std::mutex                                                               viewCacheMutex_;
    std::unordered_map<ViewKey, Ref<RHI::IRHITextureView>, ViewKeyHash, ViewKeyEq>   viewCache_;
};

} // namespace Hylux::RG::Internal
