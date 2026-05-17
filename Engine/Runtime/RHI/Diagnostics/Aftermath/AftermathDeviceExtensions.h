/// @file
/// @brief Helper that fills a VkDeviceDiagnosticsConfigCreateInfoNV with the Aftermath
///        feature flags HyluxEngine requests, ready to chain into VkDeviceCreateInfo::pNext.

#pragma once

#include <volk.h>

namespace Hylux::RHI::Vulkan
{

/// @brief Populates outInfo with the standard Aftermath feature request:
///        automatic checkpoints, resource tracking, shader debug info, shader error
///        reporting. Leaves pNext at nullptr — the caller appends it to its own chain.
void AftermathDecorateDeviceCreateInfo(VkDeviceDiagnosticsConfigCreateInfoNV& outInfo) noexcept;

} // namespace Hylux::RHI::Vulkan
