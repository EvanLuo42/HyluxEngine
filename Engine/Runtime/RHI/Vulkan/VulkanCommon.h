/// @file
/// @brief Shared includes, forward decls, and small helpers for every Vulkan backend TU.
///        Defines VK_NO_PROTOTYPES + the platform VK_USE_PLATFORM_* macro through the CMake
///        Hylux::Vulkan INTERFACE target, so this header just brings in volk.h and the
///        std::array / spdlog-style aliases the backend uses.

#pragma once

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <volk.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanInstance;
class VulkanAdapter;
class VulkanDevice;
class VulkanQueue;
class VulkanFence;
class VulkanCommandPool;
class VulkanCommandList;
class VulkanBuffer;
class VulkanTexture;
class VulkanTextureView;
class VulkanSampler;
class VulkanShaderModule;
class VulkanPipelineLayout;
class VulkanGraphicsPipeline;
class VulkanComputePipeline;
class VulkanRayTracingPipeline;
class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class VulkanBindlessHeap;
class VulkanSurface;
class VulkanSwapchain;
class VulkanQueryPool;
class VulkanAccelerationStructure;
class VulkanVendorExtension;

/// @brief Returns the textual VkResult name. Used by HYLUX_VK_RETURN_IF_FAILED.
[[nodiscard]] const char* VulkanResultToString(VkResult result) noexcept;

/// @brief Returns the textual VkObjectType name. Used by debug-utils labels.
[[nodiscard]] const char* VulkanObjectTypeToString(VkObjectType type) noexcept;

} // namespace Hylux::RHI::Vulkan

/// @brief Logs at Error severity and returns the supplied value when expr != VK_SUCCESS.
#define HYLUX_VK_RETURN_IF_FAILED(Expr, Ret)                                                       \
    do {                                                                                           \
        const VkResult hyluxVkRet_ = (Expr);                                                       \
        if (hyluxVkRet_ != VK_SUCCESS) {                                                           \
            HYLUX_LOG(::Hylux::LogRender, Error,                                                   \
                      "Vulkan call failed: {} = {} ({})", #Expr,                                   \
                      ::Hylux::RHI::Vulkan::VulkanResultToString(hyluxVkRet_),                     \
                      static_cast<int>(hyluxVkRet_));                                              \
            return (Ret);                                                                          \
        }                                                                                          \
    } while (0)

/// @brief Logs at Warn severity when expr != VK_SUCCESS but continues execution.
#define HYLUX_VK_WARN_IF_FAILED(Expr)                                                              \
    do {                                                                                           \
        const VkResult hyluxVkRet_ = (Expr);                                                       \
        if (hyluxVkRet_ != VK_SUCCESS) {                                                           \
            HYLUX_LOG(::Hylux::LogRender, Warn,                                                    \
                      "Vulkan call non-success: {} = {} ({})", #Expr,                              \
                      ::Hylux::RHI::Vulkan::VulkanResultToString(hyluxVkRet_),                     \
                      static_cast<int>(hyluxVkRet_));                                              \
        }                                                                                          \
    } while (0)
