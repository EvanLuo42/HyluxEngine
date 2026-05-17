/// @file
/// @brief VulkanCommandPool implementation.

#include "RHI/Vulkan/VulkanCommandPool.h"

#include "RHI/Vulkan/VulkanAdapter.h"
#include "RHI/Vulkan/VulkanCommandList.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"

namespace Hylux::RHI::Vulkan
{

namespace
{

std::uint32_t QueueFamilyFor(VulkanDevice* device, QueueType type)
{
    auto* adapter = device->GetVulkanAdapter();
    switch (type)
    {
        case QueueType::Graphics: return adapter->GetGraphicsQueueFamily();
        case QueueType::Compute:  return adapter->GetComputeQueueFamily();
        case QueueType::Copy:     return adapter->GetCopyQueueFamily();
        default:                  return adapter->GetGraphicsQueueFamily();
    }
}

} // namespace

VulkanCommandPool::VulkanCommandPool(VulkanDevice* device, QueueType type, CommandPoolFlags flags)
    : VulkanObject(device), type_(type)
{
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = QueueFamilyFor(device, type);
    const auto raw = static_cast<std::uint32_t>(flags);
    if (raw & static_cast<std::uint32_t>(CommandPoolFlagBits::Transient))
        ci.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if (raw & static_cast<std::uint32_t>(CommandPoolFlagBits::AllowIndividualReset))
        ci.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device->GetVkDevice(), &ci, nullptr, &pool_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateCommandPool failed");
        pool_ = VK_NULL_HANDLE;
    }
}

VulkanCommandPool::~VulkanCommandPool()
{
    if (pool_ != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device_->GetVkDevice(), pool_, nullptr);
    }
}

Ref<IRHICommandList> VulkanCommandPool::AllocateCommandList()
{
    if (pool_ == VK_NULL_HANDLE) return {};
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = pool_;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cb = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device_->GetVkDevice(), &ai, &cb) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkAllocateCommandBuffers failed");
        return {};
    }
    return MakeRef<VulkanCommandList>(device_, this, type_, cb);
}

bool VulkanCommandPool::Reset()
{
    if (pool_ == VK_NULL_HANDLE) return false;
    return vkResetCommandPool(device_->GetVkDevice(), pool_, 0) == VK_SUCCESS;
}

RHINativeHandle VulkanCommandPool::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkCommandPool,
                           reinterpret_cast<std::uint64_t>(pool_)};
}

void VulkanCommandPool::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_COMMAND_POOL,
                      reinterpret_cast<std::uint64_t>(pool_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
