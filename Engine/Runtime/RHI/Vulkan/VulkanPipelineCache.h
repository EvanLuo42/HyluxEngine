/// @file
/// @brief Vulkan implementation of IRHIPipelineCache. Wraps a single VkPipelineCache and
///        round-trips its contents through vkGetPipelineCacheData / vkCreatePipelineCache.

#pragma once

#include "RHI/IRHIPipelineCache.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <cstddef>
#include <span>
#include <vector>

namespace Hylux::RHI::Vulkan
{

/// @brief VkPipelineCache wrapper. Persistent across runs via SerializeToBlob /
///        LoadFromBlob; the engine layer owns the blob store on disk.
class VulkanPipelineCache final : public VulkanObject, public IRHIPipelineCache
{
public:
    explicit VulkanPipelineCache(VulkanDevice* device);
    ~VulkanPipelineCache() override;

    [[nodiscard]] VkPipelineCache GetVkPipelineCache() const noexcept { return cache_; }

    [[nodiscard]] std::vector<std::byte> SerializeToBlob() const override;
    bool                                 LoadFromBlob(std::span<const std::byte> blob) override;

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice*     GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }

    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    void               DestroyCache() noexcept;
    [[nodiscard]] bool CreateCache(const void* initialData, std::size_t initialDataSize);

    VkPipelineCache cache_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
