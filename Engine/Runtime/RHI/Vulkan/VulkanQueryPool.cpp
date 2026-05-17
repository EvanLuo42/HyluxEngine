/// @file
/// @brief VulkanQueryPool implementation.

#include "RHI/Vulkan/VulkanQueryPool.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"

namespace Hylux::RHI::Vulkan
{

VulkanQueryPool::VulkanQueryPool(VulkanDevice* device, QueryType type, std::uint32_t count)
    : VulkanObject(device), type_(type), count_(count)
{
    VkQueryPoolCreateInfo ci{};
    ci.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    ci.queryType  = ToVkQueryType(type);
    ci.queryCount = count;
    if (vkCreateQueryPool(device->GetVkDevice(), &ci, nullptr, &pool_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateQueryPool failed");
        pool_ = VK_NULL_HANDLE;
    }
}

VulkanQueryPool::~VulkanQueryPool()
{
    if (pool_ != VK_NULL_HANDLE)
        vkDestroyQueryPool(device_->GetVkDevice(), pool_, nullptr);
}

bool VulkanQueryPool::GetResults(std::uint32_t firstIndex, std::uint32_t count,
                                 std::span<std::uint64_t> outResults, bool wait)
{
    if (pool_ == VK_NULL_HANDLE || outResults.size() < count) return false;
    VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;
    if (wait) flags |= VK_QUERY_RESULT_WAIT_BIT;
    return vkGetQueryPoolResults(device_->GetVkDevice(), pool_, firstIndex, count,
                                 count * sizeof(std::uint64_t),
                                 outResults.data(), sizeof(std::uint64_t), flags) == VK_SUCCESS;
}

RHINativeHandle VulkanQueryPool::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::None,
                           reinterpret_cast<std::uint64_t>(pool_)};
}

void VulkanQueryPool::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_QUERY_POOL,
                      reinterpret_cast<std::uint64_t>(pool_), debugName_);
}

} // namespace Hylux::RHI::Vulkan
