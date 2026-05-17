/// @file
/// @brief Vulkan implementation of IRHIAdapter. Wraps a VkPhysicalDevice, reports its
///        feature set, and creates VulkanDevice instances.

#pragma once

#include "RHI/IRHIAdapter.h"
#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

class VulkanAdapter final : public IRHIAdapter
{
public:
    VulkanAdapter(VulkanInstance* instance, VkPhysicalDevice physicalDevice);
    ~VulkanAdapter() override = default;

    [[nodiscard]] const AdapterDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] FeatureSet GetSupportedFeatures() const noexcept override { return features_; }
    [[nodiscard]] bool IsFeatureSupported(Feature feature) const noexcept override
    {
        return features_.Has(feature);
    }

    Ref<IRHIDevice> CreateDevice(const DeviceDesc& desc) override;

    [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const noexcept { return physicalDevice_; }
    [[nodiscard]] VulkanInstance*  GetInstance()       const noexcept { return instance_; }

    [[nodiscard]] std::uint32_t  GetGraphicsQueueFamily() const noexcept { return graphicsFamily_; }
    [[nodiscard]] std::uint32_t  GetComputeQueueFamily()  const noexcept { return computeFamily_; }
    [[nodiscard]] std::uint32_t  GetCopyQueueFamily()     const noexcept { return copyFamily_; }

    // IRHIObject
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return nullptr; }
    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    void SetDebugName(std::string_view name) override { debugName_.assign(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return debugName_; }

private:
    void QueryProperties();
    void QueryQueueFamilies();
    void QueryFeatures();

    VulkanInstance*  instance_{nullptr};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    AdapterDesc      desc_{};
    FeatureSet       features_{};
    std::uint32_t    graphicsFamily_{UINT32_MAX};
    std::uint32_t    computeFamily_{UINT32_MAX};
    std::uint32_t    copyFamily_{UINT32_MAX};
    bool             isNvidia_{false};
    std::string      debugName_;
};

} // namespace Hylux::RHI::Vulkan
