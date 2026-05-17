/// @file
/// @brief VulkanFence implementation backed by a VkSemaphore in timeline mode.

#include "RHI/Vulkan/VulkanFence.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"

namespace Hylux::RHI::Vulkan
{

VulkanFence::VulkanFence(VulkanDevice* device, std::uint64_t initialValue)
    : VulkanObject(device)
{
    VkSemaphoreTypeCreateInfo typeInfo{};
    typeInfo.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    typeInfo.initialValue  = initialValue;

    VkSemaphoreCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    ci.pNext = &typeInfo;

    if (vkCreateSemaphore(device->GetVkDevice(), &ci, nullptr, &semaphore_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "VulkanFence: vkCreateSemaphore failed");
        semaphore_ = VK_NULL_HANDLE;
    }
}

VulkanFence::~VulkanFence()
{
    if (semaphore_ != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device_->GetVkDevice(), semaphore_, nullptr);
    }
}

std::uint64_t VulkanFence::GetCompletedValue() const noexcept
{
    if (semaphore_ == VK_NULL_HANDLE) return 0;
    std::uint64_t value = 0;
    vkGetSemaphoreCounterValue(device_->GetVkDevice(), semaphore_, &value);
    return value;
}

bool VulkanFence::Wait(std::uint64_t value, std::uint64_t timeoutNanos)
{
    if (semaphore_ == VK_NULL_HANDLE) return false;
    VkSemaphoreWaitInfo wi{};
    wi.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wi.semaphoreCount = 1;
    wi.pSemaphores    = &semaphore_;
    wi.pValues        = &value;
    return vkWaitSemaphores(device_->GetVkDevice(), &wi, timeoutNanos) == VK_SUCCESS;
}

bool VulkanFence::Signal(std::uint64_t value)
{
    if (semaphore_ == VK_NULL_HANDLE) return false;
    VkSemaphoreSignalInfo si{};
    si.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    si.semaphore = semaphore_;
    si.value     = value;
    return vkSignalSemaphore(device_->GetVkDevice(), &si) == VK_SUCCESS;
}

RHINativeHandle VulkanFence::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkSemaphore,
                           reinterpret_cast<std::uint64_t>(semaphore_)};
}

void VulkanFence::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_SEMAPHORE,
                      reinterpret_cast<std::uint64_t>(semaphore_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
