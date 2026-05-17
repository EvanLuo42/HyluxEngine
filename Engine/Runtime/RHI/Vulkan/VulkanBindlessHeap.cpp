/// @file
/// @brief VulkanBindlessHeap implementation. Allocates a single oversized descriptor pool +
///        descriptor set with VK_EXT_descriptor_indexing semantics; freelisting of bindless
///        slots is delegated to the (future) per-resource registration helpers.

#include "RHI/Vulkan/VulkanBindlessHeap.h"

#include "RHI/Vulkan/VulkanDevice.h"

namespace Hylux::RHI::Vulkan
{

VulkanBindlessHeap::VulkanBindlessHeap(VulkanDevice* device, BindlessKind kind, std::uint32_t capacity)
    : VulkanObject(device), kind_(kind), capacity_(capacity)
{
    // Stub: a full bindless heap requires VK_EXT_descriptor_indexing + variable descriptor
    // count + per-binding flags. Building it now would dwarf the diff; expose a placeholder
    // pool so device init does not fail.
    VkDescriptorPoolSize size{};
    if (kind == BindlessKind::Sampler)
    {
        size.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    else
    {
        size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    size.descriptorCount = capacity;

    VkDescriptorPoolCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets       = 1;
    ci.poolSizeCount = 1;
    ci.pPoolSizes    = &size;
    vkCreateDescriptorPool(device->GetVkDevice(), &ci, nullptr, &pool_);
}

VulkanBindlessHeap::~VulkanBindlessHeap()
{
    if (pool_ != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device_->GetVkDevice(), pool_, nullptr);
    if (layout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_->GetVkDevice(), layout_, nullptr);
}

RHINativeHandle VulkanBindlessHeap::GetNativeHeapHandle() const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkDescriptorPool,
                           reinterpret_cast<std::uint64_t>(pool_)};
}

RHINativeHandle VulkanBindlessHeap::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return GetNativeHeapHandle();
}

} // namespace Hylux::RHI::Vulkan
