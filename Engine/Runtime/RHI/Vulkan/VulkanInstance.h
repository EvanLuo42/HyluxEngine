/// @file
/// @brief Vulkan implementation of IRHIInstance. Owns VkInstance, the debug-utils messenger,
///        the loaded capture tool (when one is enabled), and the enumerated adapter list.

#pragma once

#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/IRHIInstance.h"
#include "RHI/RHIInstanceDesc.h"
#include "RHI/Vulkan/VulkanAdapter.h"
#include "RHI/Vulkan/VulkanCommon.h"

#include <memory>
#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanAdapter;

/// @brief Concrete IRHIInstance for Vulkan. Constructed via CreateRHIInstance().
class VulkanInstance final : public IRHIInstance
{
public:
    explicit VulkanInstance(const InstanceDesc& desc);
    ~VulkanInstance() override;

    /// @brief Returns true if vkCreateInstance succeeded and adapters were enumerated.
    [[nodiscard]] bool IsValid() const noexcept { return instance_ != VK_NULL_HANDLE; }

    [[nodiscard]] DeviceType GetDeviceType() const noexcept override { return DeviceType::Vulkan; }

    [[nodiscard]] std::uint32_t GetAdapterCount() const noexcept override;
    [[nodiscard]] Ref<IRHIAdapter> GetAdapter(std::uint32_t index) const override;
    [[nodiscard]] Ref<IRHIAdapter> GetDefaultAdapter() const override;

    [[nodiscard]] IGraphicsCaptureTool* GetCaptureTool() const noexcept override
    {
        return captureTool_.Get();
    }

    [[nodiscard]] VkInstance GetVkInstance() const noexcept { return instance_; }
    [[nodiscard]] const InstanceDesc& GetDesc() const noexcept { return desc_; }
    [[nodiscard]] bool HasDebugUtils() const noexcept { return hasDebugUtils_; }

    // IRHIObject
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return nullptr; }
    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    void SetDebugName(std::string_view name) override { debugName_.assign(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return debugName_; }

private:
    bool Initialize();
    void Shutdown() noexcept;

    InstanceDesc                desc_;
    VkInstance                  instance_{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT    debugMessenger_{VK_NULL_HANDLE};
    bool                        hasDebugUtils_{false};
    std::vector<Ref<VulkanAdapter>> adapters_;
    Ref<IGraphicsCaptureTool>   captureTool_;
    std::string                 debugName_;
};

} // namespace Hylux::RHI::Vulkan
