/// @file
/// @brief Thin wrappers around VK_EXT_debug_utils calls used by the backend.

#pragma once

#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

/// @brief Attaches a debug name to a Vulkan handle if VK_EXT_debug_utils is enabled on
///        the device. No-op on devices without the extension.
void SetObjectName(VkDevice device, VkObjectType type, std::uint64_t handle,
                   std::string_view name) noexcept;

} // namespace Hylux::RHI::Vulkan
