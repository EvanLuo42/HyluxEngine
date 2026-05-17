/// @file
/// @brief Vulkan implementation of IRHISampler.

#pragma once

#include "RHI/IRHISampler.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanSampler final : public VulkanObject, public IRHISampler
{
public:
    VulkanSampler(VulkanDevice* device, const SamplerDesc& desc);
    ~VulkanSampler() override;

    [[nodiscard]] bool IsValid() const noexcept { return sampler_ != VK_NULL_HANDLE; }

    [[nodiscard]] const SamplerDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] BindlessIndex GetBindlessIndex() const noexcept override
    {
        return BindlessIndex::Invalid;
    }

    [[nodiscard]] VkSampler GetVkSampler() const noexcept { return sampler_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    SamplerDesc desc_;
    VkSampler   sampler_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
