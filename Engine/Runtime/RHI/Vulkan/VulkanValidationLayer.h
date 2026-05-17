/// @file
/// @brief Helpers for selecting validation layers and installing a debug-utils messenger
///        that forwards Vulkan validation messages to HYLUX_LOG(LogRender, ...).

#pragma once

#include "RHI/RHIValidation.h"
#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

/// @brief Returns true if VK_LAYER_KHRONOS_validation appears in the enumerated layer list.
[[nodiscard]] bool IsValidationLayerAvailable() noexcept;

/// @brief Creates a debug-utils messenger that routes Vulkan messages into Hylux logging.
///        Returns VK_NULL_HANDLE if VK_EXT_debug_utils is not enabled or if creation fails.
[[nodiscard]] VkDebugUtilsMessengerEXT CreateDebugUtilsMessenger(
    VkInstance instance, GapiValidationLevel level) noexcept;

/// @brief Destroys a previously created messenger. No-op for VK_NULL_HANDLE.
void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) noexcept;

} // namespace Hylux::RHI::Vulkan
