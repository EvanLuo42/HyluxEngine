/// @file
/// @brief RGTransientResourcePool implementation.

#include "RenderGraph/Internal/RGTransientResourcePool.h"

#include <algorithm>
#include <utility>

namespace Hylux::RG::Internal
{

RGTransientResourcePool::RGTransientResourcePool(TextureFactory textureFactory, BufferFactory bufferFactory,
                                                 std::uint32_t framesInFlight)
    : textureFactory_(std::move(textureFactory)),
      bufferFactory_(std::move(bufferFactory)),
      framesInFlight_(framesInFlight == 0 ? 1u : framesInFlight)
{}

RGTransientResourcePool::~RGTransientResourcePool() = default;

bool RGTransientResourcePool::DescEquals(const RHI::TextureDesc& a, const RHI::TextureDesc& b) noexcept
{
    return a.dimension == b.dimension && a.format == b.format && a.extent.width == b.extent.width
           && a.extent.height == b.extent.height && a.extent.depth == b.extent.depth
           && a.mipLevels == b.mipLevels && a.arrayLayers == b.arrayLayers
           && a.sampleCount == b.sampleCount && a.usage == b.usage && a.memory == b.memory
           && a.aliasable == b.aliasable;
}

bool RGTransientResourcePool::DescEquals(const RHI::BufferDesc& a, const RHI::BufferDesc& b) noexcept
{
    return a.size == b.size && a.usage == b.usage && a.memory == b.memory
           && a.structureStride == b.structureStride && a.bindlessSrv == b.bindlessSrv
           && a.bindlessUav == b.bindlessUav && a.bindlessCbv == b.bindlessCbv
           && a.aliasable == b.aliasable;
}

bool RGTransientResourcePool::IsReusable(std::uint64_t lastUsedFrame, std::uint64_t currentFrame) const noexcept
{
    // Reusable once the framesInFlight-deep submission that borrowed it has retired on
    // the GPU. lastUsedFrame + framesInFlight_ cannot overflow in practice (framesInFlight
    // is u32, lastUsedFrame is bounded by the running frame counter).
    return lastUsedFrame + framesInFlight_ <= currentFrame;
}

Ref<RHI::IRHITexture> RGTransientResourcePool::AcquireTexture(const RHI::TextureDesc& desc,
                                                              std::uint64_t           frameId)
{
    for (TextureEntry& entry : textures_)
    {
        if (entry.texture && DescEquals(entry.desc, desc) && IsReusable(entry.lastUsedFrame, frameId))
        {
            entry.lastUsedFrame = frameId;
            ++stats_.textureReuseCount;
            return entry.texture;
        }
    }

    Ref<RHI::IRHITexture> fresh = textureFactory_ ? textureFactory_(desc) : Ref<RHI::IRHITexture>{};
    if (!fresh)
    {
        return Ref<RHI::IRHITexture>{};
    }

    TextureEntry inserted{};
    inserted.texture       = fresh;
    inserted.desc          = desc;
    inserted.lastUsedFrame = frameId;
    textures_.push_back(std::move(inserted));
    ++stats_.textureCreateCount;
    return fresh;
}

Ref<RHI::IRHIBuffer> RGTransientResourcePool::AcquireBuffer(const RHI::BufferDesc& desc,
                                                            std::uint64_t          frameId)
{
    for (BufferEntry& entry : buffers_)
    {
        if (entry.buffer && DescEquals(entry.desc, desc) && IsReusable(entry.lastUsedFrame, frameId))
        {
            entry.lastUsedFrame = frameId;
            ++stats_.bufferReuseCount;
            return entry.buffer;
        }
    }

    Ref<RHI::IRHIBuffer> fresh = bufferFactory_ ? bufferFactory_(desc) : Ref<RHI::IRHIBuffer>{};
    if (!fresh)
    {
        return Ref<RHI::IRHIBuffer>{};
    }

    BufferEntry inserted{};
    inserted.buffer        = fresh;
    inserted.desc          = desc;
    inserted.lastUsedFrame = frameId;
    buffers_.push_back(std::move(inserted));
    ++stats_.bufferCreateCount;
    return fresh;
}

void RGTransientResourcePool::EndFrame(std::uint64_t frameId, std::uint32_t maxIdleFrames)
{
    if (frameId <= maxIdleFrames)
    {
        return;
    }
    const std::uint64_t cutoff = frameId - maxIdleFrames;

    std::erase_if(textures_, [cutoff](const TextureEntry& e) { return e.lastUsedFrame < cutoff; });
    std::erase_if(buffers_,  [cutoff](const BufferEntry&  e) { return e.lastUsedFrame < cutoff; });
}

void RGTransientResourcePool::Reset()
{
    textures_.clear();
    buffers_.clear();
}

RGTransientResourcePool::Stats RGTransientResourcePool::GetStats() const noexcept
{
    Stats result            = stats_;
    result.textureEntries   = textures_.size();
    result.bufferEntries    = buffers_.size();
    return result;
}

} // namespace Hylux::RG::Internal
