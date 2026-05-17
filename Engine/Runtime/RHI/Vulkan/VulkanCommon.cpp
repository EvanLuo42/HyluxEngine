/// @file
/// @brief Implementation of small textual helpers shared across the Vulkan backend.

#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

const char* VulkanResultToString(VkResult result) noexcept
{
    switch (result)
    {
        case VK_SUCCESS:                                            return "VK_SUCCESS";
        case VK_NOT_READY:                                          return "VK_NOT_READY";
        case VK_TIMEOUT:                                            return "VK_TIMEOUT";
        case VK_EVENT_SET:                                          return "VK_EVENT_SET";
        case VK_EVENT_RESET:                                        return "VK_EVENT_RESET";
        case VK_INCOMPLETE:                                         return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:                        return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:                                  return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:                            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:                            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:                          return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:                             return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:                              return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:                                      return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:                                return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:                                     return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:                              return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:                            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR:                            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:                                    return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:                                    return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:                             return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:                         return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:                      return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
        default:                                                    return "VK_RESULT_UNKNOWN";
    }
}

const char* VulkanObjectTypeToString(VkObjectType type) noexcept
{
    switch (type)
    {
        case VK_OBJECT_TYPE_INSTANCE:                return "Instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:         return "PhysicalDevice";
        case VK_OBJECT_TYPE_DEVICE:                  return "Device";
        case VK_OBJECT_TYPE_QUEUE:                   return "Queue";
        case VK_OBJECT_TYPE_SEMAPHORE:               return "Semaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:          return "CommandBuffer";
        case VK_OBJECT_TYPE_FENCE:                   return "Fence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:           return "DeviceMemory";
        case VK_OBJECT_TYPE_BUFFER:                  return "Buffer";
        case VK_OBJECT_TYPE_IMAGE:                   return "Image";
        case VK_OBJECT_TYPE_EVENT:                   return "Event";
        case VK_OBJECT_TYPE_QUERY_POOL:              return "QueryPool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:             return "BufferView";
        case VK_OBJECT_TYPE_IMAGE_VIEW:              return "ImageView";
        case VK_OBJECT_TYPE_SHADER_MODULE:           return "ShaderModule";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:          return "PipelineCache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:         return "PipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS:             return "RenderPass";
        case VK_OBJECT_TYPE_PIPELINE:                return "Pipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:   return "DescriptorSetLayout";
        case VK_OBJECT_TYPE_SAMPLER:                 return "Sampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:         return "DescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:          return "DescriptorSet";
        case VK_OBJECT_TYPE_FRAMEBUFFER:             return "Framebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:            return "CommandPool";
        case VK_OBJECT_TYPE_SURFACE_KHR:             return "Surface";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:           return "Swapchain";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "DebugUtilsMessenger";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR: return "AccelerationStructure";
        default:                                     return "Unknown";
    }
}

} // namespace Hylux::RHI::Vulkan
