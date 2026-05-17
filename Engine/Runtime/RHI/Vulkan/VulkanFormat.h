/// @file
/// @brief Lookup helpers translating Hylux::RHI::Format <-> VkFormat.

#pragma once

#include "RHI/RHIFormat.h"
#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

/// @brief Returns the VkFormat corresponding to the given Hylux Format. Returns VK_FORMAT_UNDEFINED
///        for Format::Unknown and any format the backend does not yet map.
[[nodiscard]] VkFormat ToVkFormat(Format format) noexcept;

/// @brief Reverse mapping. Returns Format::Unknown for VkFormat values without a Hylux equivalent.
[[nodiscard]] Format FromVkFormat(VkFormat format) noexcept;

/// @brief Returns the VkImageAspectFlags appropriate for the format (depth/stencil/color).
[[nodiscard]] VkImageAspectFlags FormatAspect(Format format) noexcept;

} // namespace Hylux::RHI::Vulkan
