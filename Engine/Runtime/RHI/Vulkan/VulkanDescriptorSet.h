/// @file
/// @brief Vulkan implementations of IRHIDescriptorSetLayout and IRHIDescriptorSet.

#pragma once

#include "RHI/IRHIDescriptorSet.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanDescriptorSetLayout final : public VulkanObject, public IRHIDescriptorSetLayout
{
public:
    VulkanDescriptorSetLayout(VulkanDevice* device, const DescriptorSetLayoutDesc& desc);
    ~VulkanDescriptorSetLayout() override;

    [[nodiscard]] bool IsValid() const noexcept { return layout_ != VK_NULL_HANDLE; }
    [[nodiscard]] const DescriptorSetLayoutDesc& GetDesc() const noexcept override { return desc_; }

    [[nodiscard]] VkDescriptorSetLayout GetVkLayout() const noexcept { return layout_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    DescriptorSetLayoutDesc desc_;  // bindings span — caller-owned, we copy the data
    std::vector<DescriptorSetLayoutBinding> bindingsCopy_;
    VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
};

class VulkanDescriptorSet final : public VulkanObject, public IRHIDescriptorSet
{
public:
    VulkanDescriptorSet(VulkanDevice* device, VulkanDescriptorSetLayout* layout,
                        VkDescriptorPool pool, VkDescriptorSet set);
    ~VulkanDescriptorSet() override;

    [[nodiscard]] IRHIDescriptorSetLayout* GetLayout() const noexcept override;

    void Update(std::span<const DescriptorWrite> writes) override;

    [[nodiscard]] VkDescriptorSet GetVkDescriptorSet() const noexcept { return set_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    Ref<VulkanDescriptorSetLayout> layout_;
    VkDescriptorPool               pool_{VK_NULL_HANDLE};
    VkDescriptorSet                set_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
