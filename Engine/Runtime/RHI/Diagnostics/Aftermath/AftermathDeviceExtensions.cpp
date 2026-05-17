/// @file
/// @brief AftermathDecorateDeviceCreateInfo implementation.

#include "RHI/Diagnostics/Aftermath/AftermathDeviceExtensions.h"

namespace Hylux::RHI::Vulkan
{

void AftermathDecorateDeviceCreateInfo(VkDeviceDiagnosticsConfigCreateInfoNV& info) noexcept
{
    info.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
    info.pNext = nullptr;
    info.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV
               | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV
               | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV
               | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV;
}

} // namespace Hylux::RHI::Vulkan
