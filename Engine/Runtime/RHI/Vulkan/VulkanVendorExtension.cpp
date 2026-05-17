/// @file
/// @brief VulkanVendorExtension implementation (stub).

#include "RHI/Vulkan/VulkanVendorExtension.h"

namespace Hylux::RHI::Vulkan
{

VulkanVendorExtension::VulkanVendorExtension(VulkanDevice* device, VendorExtensionKind kind,
                                             void* nativeCtx)
    : VulkanObject(device), kind_(kind), nativeCtx_(nativeCtx) {}

} // namespace Hylux::RHI::Vulkan
