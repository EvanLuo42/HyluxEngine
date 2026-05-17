/// @file
/// @brief Out-of-line definitions for the VulkanObject helper.

#include "RHI/Vulkan/VulkanObject.h"

#include "RHI/Vulkan/VulkanDevice.h"

namespace Hylux::RHI::Vulkan
{

IRHIDevice* VulkanObject::GetVulkanDeviceAsRhi() const noexcept
{
    return static_cast<IRHIDevice*>(device_);
}

} // namespace Hylux::RHI::Vulkan
