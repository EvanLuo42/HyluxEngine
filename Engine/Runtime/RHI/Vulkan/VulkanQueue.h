/// @file
/// @brief Vulkan implementation of IRHIQueue. Wraps VkQueue + the queue family index it
///        belongs to and translates Submit/Present descriptors to vkQueueSubmit2 / present.

#pragma once

#include "RHI/IRHIQueue.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanQueue final : public VulkanObject, public IRHIQueue
{
public:
    VulkanQueue(VulkanDevice* device, QueueType type, std::uint32_t familyIndex,
                std::uint32_t indexWithinType, VkQueue queue);

    [[nodiscard]] QueueType     GetType() const noexcept override  { return type_; }
    [[nodiscard]] std::uint32_t GetIndex() const noexcept override { return indexWithinType_; }

    bool Submit(std::span<const SubmitDesc> submits) override;
    bool Present(const PresentDesc& presentDesc) override;
    bool WaitIdle() override;

    [[nodiscard]] std::uint32_t GetFamilyIndex() const noexcept { return familyIndex_; }
    [[nodiscard]] VkQueue       GetVkQueue()    const noexcept { return queue_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    // VulkanObject + IRHIObject diamond disambiguation
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    QueueType     type_;
    std::uint32_t familyIndex_;
    std::uint32_t indexWithinType_;
    VkQueue       queue_;
};

} // namespace Hylux::RHI::Vulkan
