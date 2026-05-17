/// @file
/// @brief Helper mixin storing the device back-pointer and debug name for Vulkan
///        IRHIObject implementations. Intentionally does NOT inherit from IRHIObject —
///        concrete types inherit from the specific IRHIXxx interface (which derives from
///        IRHIObject) and pull in this helper via plain (non-virtual) multiple inheritance
///        to share storage and a couple of small methods.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

/// @brief Shared state for every Vulkan backend object. Holds a raw VulkanDevice pointer
///        (the device outlives every resource by contract) and the debug-name string.
class VulkanObject
{
public:
    explicit VulkanObject(VulkanDevice* device) noexcept : device_(device) {}

    [[nodiscard]] IRHIDevice* GetVulkanDeviceAsRhi() const noexcept;

    void SetDebugNameImpl(std::string_view name)
    {
        debugName_.assign(name);
        OnDebugNameChanged();
    }

    [[nodiscard]] std::string_view GetDebugNameImpl() const noexcept { return debugName_; }

protected:
    /// @brief Hook invoked after the debug name buffer is updated. Subclasses override to
    ///        forward the name to vkSetDebugUtilsObjectNameEXT for their VK handle.
    virtual void OnDebugNameChanged() {}

    VulkanDevice* device_{nullptr};
    std::string   debugName_;
};

} // namespace Hylux::RHI::Vulkan
