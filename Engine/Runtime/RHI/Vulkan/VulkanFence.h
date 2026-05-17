/// @file
/// @brief Vulkan implementation of IRHIFence using a timeline semaphore (VK_KHR_timeline_semaphore).

#pragma once

#include "RHI/IRHIFence.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanFence final : public VulkanObject, public IRHIFence
{
public:
    VulkanFence(VulkanDevice* device, std::uint64_t initialValue);
    ~VulkanFence() override;

    [[nodiscard]] std::uint64_t GetCompletedValue() const noexcept override;
    bool Wait(std::uint64_t value, std::uint64_t timeoutNanos = kFenceWaitInfinite) override;
    bool Signal(std::uint64_t value) override;

    [[nodiscard]] VkSemaphore GetVkSemaphore() const noexcept { return semaphore_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    VkSemaphore semaphore_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
