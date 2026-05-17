/// @file
/// @brief Implementation of the debug-utils name setter.

#include "RHI/Vulkan/VulkanDebugUtils.h"

namespace Hylux::RHI::Vulkan
{

void SetObjectName(VkDevice device, VkObjectType type, std::uint64_t handle,
                   std::string_view name) noexcept
{
    if (device == VK_NULL_HANDLE || handle == 0 || vkSetDebugUtilsObjectNameEXT == nullptr)
    {
        return;
    }

    // VkDebugUtilsObjectNameInfoEXT takes a NUL-terminated const char*. string_view doesn't
    // guarantee one, so copy into a small stack buffer when possible.
    char stackBuf[256];
    const std::size_t copyLen = name.size() < sizeof(stackBuf) - 1 ? name.size() : sizeof(stackBuf) - 1;
    std::memcpy(stackBuf, name.data(), copyLen);
    stackBuf[copyLen] = '\0';

    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType   = type;
    info.objectHandle = handle;
    info.pObjectName  = stackBuf;
    vkSetDebugUtilsObjectNameEXT(device, &info);
}

} // namespace Hylux::RHI::Vulkan
