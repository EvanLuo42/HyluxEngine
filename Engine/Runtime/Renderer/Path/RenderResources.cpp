/// @file
/// @brief RenderResources implementation: name-keyed persistent texture pool with
///        idle eviction. The pool reallocates a slot when the requested TextureDesc
///        differs structurally from the cached one (typically a viewport resize); a
///        per-slot lastUsedFrameId stamp drives EndFrame's eviction.

#include "Renderer/Path/RenderResources.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHITexture.h"

#include <string>
#include <utility>

namespace Hylux::Renderer
{

RenderResources::RenderResources(RHI::IRHIDevice* device) noexcept : device_(device) {}

RenderResources::~RenderResources() = default;

bool RenderResources::DescEquals(const RHI::TextureDesc& a, const RHI::TextureDesc& b) noexcept
{
    return a.dimension == b.dimension && a.format == b.format && a.extent.width == b.extent.width &&
           a.extent.height == b.extent.height && a.extent.depth == b.extent.depth && a.mipLevels == b.mipLevels &&
           a.arrayLayers == b.arrayLayers && a.sampleCount == b.sampleCount && a.usage == b.usage &&
           a.memory == b.memory && a.aliasable == b.aliasable;
}

RHI::IRHITexture* RenderResources::GetOrCreateTexture(std::string_view name, const RHI::TextureDesc& desc,
                                                     const std::uint64_t renderFrameId)
{
    if (device_ == nullptr || name.empty())
    {
        return nullptr;
    }

    const std::string key{name};
    auto              it = slots_.find(key);
    if (it != slots_.end() && it->second.texture && DescEquals(it->second.desc, desc))
    {
        it->second.lastUsedFrameId = renderFrameId;
        return it->second.texture.Get();
    }

    Ref<RHI::IRHITexture> texture = device_->CreateTexture(desc);
    if (!texture)
    {
        HYLUX_LOG_ERROR(LogRender, "RenderResources: CreateTexture failed for slot '{}'", key);
        return nullptr;
    }
    texture->SetDebugName(key);

    if (it == slots_.end())
    {
        Slot slot{};
        slot.texture         = std::move(texture);
        slot.desc            = desc;
        slot.lastUsedFrameId = renderFrameId;
        auto [newIt, _]      = slots_.emplace(key, std::move(slot));
        return newIt->second.texture.Get();
    }

    it->second.texture         = std::move(texture);
    it->second.desc            = desc;
    it->second.lastUsedFrameId = renderFrameId;
    return it->second.texture.Get();
}

RHI::IRHITexture* RenderResources::FindTexture(std::string_view name) const noexcept
{
    const std::string key{name};
    const auto        it = slots_.find(key);
    return it == slots_.end() ? nullptr : it->second.texture.Get();
}

void RenderResources::Reset()
{
    slots_.clear();
}

void RenderResources::EndFrame(const std::uint64_t renderFrameId, const std::uint32_t idleFrames)
{
    if (renderFrameId <= idleFrames)
    {
        return;
    }
    const std::uint64_t cutoff = renderFrameId - idleFrames;
    for (auto it = slots_.begin(); it != slots_.end();)
    {
        if (it->second.lastUsedFrameId < cutoff)
        {
            it = slots_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace Hylux::Renderer
