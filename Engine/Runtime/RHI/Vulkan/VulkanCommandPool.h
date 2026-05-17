/// @file
/// @brief Vulkan implementation of IRHICommandPool. Wraps a VkCommandPool and a free-list of
///        allocated VulkanCommandList wrappers.

#pragma once

#include "RHI/IRHICommandPool.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanCommandPool final : public VulkanObject, public IRHICommandPool
{
public:
    VulkanCommandPool(VulkanDevice* device, QueueType type, CommandPoolFlags flags);
    ~VulkanCommandPool() override;

    [[nodiscard]] QueueType GetQueueType() const noexcept override { return type_; }

    Ref<IRHICommandList> AllocateCommandList() override;
    bool Reset() override;

    [[nodiscard]] VkCommandPool GetVkCommandPool() const noexcept { return pool_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    QueueType     type_;
    VkCommandPool pool_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
