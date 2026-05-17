/// @file
/// @brief VulkanPipelineCache implementation.

#include "RHI/Vulkan/VulkanPipelineCache.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"

#include <cstring>

namespace Hylux::RHI::Vulkan
{

VulkanPipelineCache::VulkanPipelineCache(VulkanDevice* device) : VulkanObject(device)
{
    CreateCache(nullptr, 0);
}

VulkanPipelineCache::~VulkanPipelineCache()
{
    DestroyCache();
}

bool VulkanPipelineCache::CreateCache(const void* initialData, std::size_t initialDataSize)
{
    VkPipelineCacheCreateInfo ci{};
    ci.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    ci.initialDataSize = initialDataSize;
    ci.pInitialData    = initialData;

    if (vkCreatePipelineCache(device_->GetVkDevice(), &ci, nullptr, &cache_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreatePipelineCache failed");
        cache_ = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void VulkanPipelineCache::DestroyCache() noexcept
{
    if (cache_ != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(device_->GetVkDevice(), cache_, nullptr);
        cache_ = VK_NULL_HANDLE;
    }
}

std::vector<std::byte> VulkanPipelineCache::SerializeToBlob() const
{
    if (cache_ == VK_NULL_HANDLE)
    {
        return {};
    }
    std::size_t dataSize = 0;
    if (vkGetPipelineCacheData(device_->GetVkDevice(), cache_, &dataSize, nullptr) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkGetPipelineCacheData size query failed");
        return {};
    }
    if (dataSize == 0)
    {
        return {};
    }
    std::vector<std::byte> blob(dataSize);
    if (vkGetPipelineCacheData(device_->GetVkDevice(), cache_, &dataSize, blob.data()) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkGetPipelineCacheData data fetch failed");
        return {};
    }
    blob.resize(dataSize);
    return blob;
}

bool VulkanPipelineCache::LoadFromBlob(std::span<const std::byte> blob)
{
    DestroyCache();
    const bool ok = CreateCache(blob.empty() ? nullptr : blob.data(), blob.size());
    if (!ok)
    {
        // Validation rejected the blob (driver / GPU mismatch). Rebuild an empty cache so
        // subsequent pipeline creates still succeed.
        CreateCache(nullptr, 0);
        return false;
    }
    return true;
}

RHINativeHandle VulkanPipelineCache::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPipelineCache,
                           reinterpret_cast<std::uint64_t>(cache_)};
}

void VulkanPipelineCache::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_PIPELINE_CACHE,
                      reinterpret_cast<std::uint64_t>(cache_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
