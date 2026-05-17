/// @file
/// @brief RGResourceRegistry implementation.

#include "RenderGraph/Internal/RGResourceRegistry.h"

#include "Core/Utils/Hash.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHITexture.h"

#include <cassert>

namespace Hylux::RG::Internal
{

namespace
{

constexpr std::uint8_t kFlagSrv = 1u << 0;
constexpr std::uint8_t kFlagUav = 1u << 1;
constexpr std::uint8_t kFlagRtv = 1u << 2;
constexpr std::uint8_t kFlagDsv = 1u << 3;

[[nodiscard]] std::uint8_t PackViewFlags(const RHI::TextureViewDesc& desc) noexcept
{
    std::uint8_t flags = 0;
    if (desc.isShaderResourceView)  flags |= kFlagSrv;
    if (desc.isUnorderedAccessView) flags |= kFlagUav;
    if (desc.isRenderTargetView)    flags |= kFlagRtv;
    if (desc.isDepthStencilView)    flags |= kFlagDsv;
    return flags;
}

} // namespace

RGResourceRegistry::RGResourceRegistry(RHI::IRHIDevice* device,
                                       std::vector<RGTextureNode>* textures,
                                       std::vector<RGBufferNode>* buffers) noexcept
    : device_(device), textures_(textures), buffers_(buffers)
{
}

RHI::IRHITexture* RGResourceRegistry::GetTexture(std::uint32_t textureIndex) const
{
    assert(textures_ != nullptr);
    assert(textureIndex < textures_->size());
    return (*textures_)[textureIndex].realized.Get();
}

RHI::IRHIBuffer* RGResourceRegistry::GetBuffer(std::uint32_t bufferIndex) const
{
    assert(buffers_ != nullptr);
    assert(bufferIndex < buffers_->size());
    return (*buffers_)[bufferIndex].realized.Get();
}

RHI::IRHITextureView* RGResourceRegistry::GetTextureView(std::uint32_t textureIndex,
                                                         const RHI::TextureViewDesc& desc)
{
    assert(textures_ != nullptr);
    assert(textureIndex < textures_->size());

    const ViewKey key{
        textureIndex,
        desc.dimension,
        desc.format,
        desc.range,
        PackViewFlags(desc),
    };

    if (auto it = viewCache_.find(key); it != viewCache_.end())
    {
        return it->second.Get();
    }

    RHI::IRHITexture* texture = (*textures_)[textureIndex].realized.Get();
    assert(texture != nullptr && "GetTextureView called before texture was realized");
    Ref<RHI::IRHITextureView> view = device_->CreateTextureView(texture, desc);
    RHI::IRHITextureView*     raw  = view.Get();
    viewCache_.emplace(key, std::move(view));
    return raw;
}

void RGResourceRegistry::Reset() noexcept
{
    viewCache_.clear();
}

std::size_t RGResourceRegistry::ViewKeyHash::operator()(const ViewKey& key) const noexcept
{
    std::uint64_t h = Hash::Fnv1a64Offset;
    const auto mix = [&h](const void* data, std::size_t size)
    {
        h = Hash::Fnv1a64(reinterpret_cast<const char*>(data), size, h);
    };
    mix(&key.textureIndex,           sizeof(key.textureIndex));
    mix(&key.dimension,              sizeof(key.dimension));
    mix(&key.format,                 sizeof(key.format));
    mix(&key.range.baseMipLevel,     sizeof(key.range.baseMipLevel));
    mix(&key.range.mipLevelCount,    sizeof(key.range.mipLevelCount));
    mix(&key.range.baseArrayLayer,   sizeof(key.range.baseArrayLayer));
    mix(&key.range.arrayLayerCount,  sizeof(key.range.arrayLayerCount));
    mix(&key.viewFlags,              sizeof(key.viewFlags));
    return static_cast<std::size_t>(h);
}

bool RGResourceRegistry::ViewKeyEq::operator()(const ViewKey& a, const ViewKey& b) const noexcept
{
    return a.textureIndex          == b.textureIndex
        && a.dimension             == b.dimension
        && a.format                == b.format
        && a.range.baseMipLevel    == b.range.baseMipLevel
        && a.range.mipLevelCount   == b.range.mipLevelCount
        && a.range.baseArrayLayer  == b.range.baseArrayLayer
        && a.range.arrayLayerCount == b.range.arrayLayerCount
        && a.viewFlags             == b.viewFlags;
}

} // namespace Hylux::RG::Internal
