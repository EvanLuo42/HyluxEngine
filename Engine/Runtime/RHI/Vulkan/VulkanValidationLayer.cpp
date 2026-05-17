/// @file
/// @brief Validation layer + debug-utils messenger wiring.

#include "RHI/Vulkan/VulkanValidationLayer.h"

#include <cstring>

namespace Hylux::RHI::Vulkan
{

namespace
{

constexpr const char* kKhronosValidationLayer = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/)
{
    if (data == nullptr || data->pMessage == nullptr)
    {
        return VK_FALSE;
    }

    const char* msg = data->pMessage;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "[VVL] {}", msg);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn, "[VVL] {}", msg);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        HYLUX_LOG(::Hylux::LogRender, Info, "[VVL] {}", msg);
    }
    else
    {
        HYLUX_LOG(::Hylux::LogRender, Debug, "[VVL] {}", msg);
    }
    return VK_FALSE;
}

} // namespace

bool IsValidationLayerAvailable() noexcept
{
    std::uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0)
    {
        return false;
    }
    std::vector<VkLayerProperties> layers(count);
    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS)
    {
        return false;
    }
    for (const auto& layer : layers)
    {
        if (std::strcmp(layer.layerName, kKhronosValidationLayer) == 0)
        {
            return true;
        }
    }
    return false;
}

VkDebugUtilsMessengerEXT CreateDebugUtilsMessenger(VkInstance instance,
                                                   GapiValidationLevel level) noexcept
{
    if (instance == VK_NULL_HANDLE || level == GapiValidationLevel::Off ||
        vkCreateDebugUtilsMessengerEXT == nullptr)
    {
        return VK_NULL_HANDLE;
    }

    VkDebugUtilsMessageSeverityFlagsEXT severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    if (level >= GapiValidationLevel::Standard)
    {
        severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    }
    if (level >= GapiValidationLevel::Gpu)
    {
        severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }

    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = severity;
    info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = &DebugMessengerCallback;

    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    if (vkCreateDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "Vulkan: failed to install debug-utils messenger");
        return VK_NULL_HANDLE;
    }
    return messenger;
}

void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) noexcept
{
    if (messenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }
}

} // namespace Hylux::RHI::Vulkan
