/// @file
/// @brief Vulkan implementation of IRHIBindlessHeap. The Vulkan analog of a D3D12 shader-
///        visible heap is the descriptor pool + descriptor set indexed by descriptor-indexing.

#pragma once

#include "RHI/IRHIBindlessHeap.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanBindlessHeap final : public VulkanObject, public IRHIBindlessHeap
{
public:
    VulkanBindlessHeap(VulkanDevice* device, BindlessKind kind, std::uint32_t capacity);
    ~VulkanBindlessHeap() override;

    [[nodiscard]] BindlessKind     GetKind()      const noexcept override { return kind_; }
    [[nodiscard]] std::uint32_t    GetCapacity()  const noexcept override { return capacity_; }
    [[nodiscard]] std::uint32_t    GetUsedCount() const noexcept override { return used_; }
    [[nodiscard]] RHINativeHandle  GetNativeHeapHandle() const noexcept override;

    [[nodiscard]] VkDescriptorSet  GetVkDescriptorSet() const noexcept { return set_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override {}

private:
    BindlessKind          kind_;
    std::uint32_t         capacity_{0};
    std::uint32_t         used_{0};
    VkDescriptorPool      pool_{VK_NULL_HANDLE};
    VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
    VkDescriptorSet       set_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
