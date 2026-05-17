/// @file
/// @brief VulkanAdapter implementation. Queries VkPhysicalDevice properties/features/queue
///        families and is the factory for VulkanDevice.

#include "RHI/Vulkan/VulkanAdapter.h"

#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanInstance.h"

namespace Hylux::RHI::Vulkan
{

namespace
{

constexpr std::uint32_t kNvidiaVendorId = 0x10DE;
constexpr std::uint32_t kAmdVendorId    = 0x1002;
constexpr std::uint32_t kIntelVendorId  = 0x8086;

} // namespace

VulkanAdapter::VulkanAdapter(VulkanInstance* instance, VkPhysicalDevice physicalDevice)
    : instance_(instance), physicalDevice_(physicalDevice)
{
    QueryProperties();
    QueryQueueFamilies();
    QueryFeatures();
}

void VulkanAdapter::QueryProperties()
{
    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkGetPhysicalDeviceProperties2(physicalDevice_, &props2);
    const VkPhysicalDeviceProperties& props = props2.properties;

    desc_.name     = props.deviceName;
    desc_.vendorId = props.vendorID;
    desc_.deviceId = props.deviceID;
    desc_.type     = DeviceType::Vulkan;

    isNvidia_ = (props.vendorID == kNvidiaVendorId);

    AdapterFlags flags = AdapterFlags::None;
    switch (props.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            flags = AdapterFlags::Discrete; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            flags = AdapterFlags::Integrated; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            flags = AdapterFlags::Virtual; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            flags = AdapterFlags::Software; break;
        default: break;
    }
    desc_.flags = flags;

    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
    std::uint64_t dedicated = 0;
    std::uint64_t shared    = 0;
    for (std::uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            dedicated += memProps.memoryHeaps[i].size;
        }
        else
        {
            shared += memProps.memoryHeaps[i].size;
        }
    }
    desc_.dedicatedVideoMemory = dedicated;
    desc_.systemMemory         = shared;
}

void VulkanAdapter::QueryQueueFamilies()
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, families.data());

    // Prefer dedicated families: copy-only first, then compute-only, then graphics.
    for (std::uint32_t i = 0; i < count; ++i)
    {
        const auto f = families[i].queueFlags;
        if ((f & VK_QUEUE_TRANSFER_BIT) &&
            !(f & VK_QUEUE_GRAPHICS_BIT) && !(f & VK_QUEUE_COMPUTE_BIT))
        {
            copyFamily_ = i;
            break;
        }
    }
    for (std::uint32_t i = 0; i < count; ++i)
    {
        const auto f = families[i].queueFlags;
        if ((f & VK_QUEUE_COMPUTE_BIT) && !(f & VK_QUEUE_GRAPHICS_BIT))
        {
            computeFamily_ = i;
            break;
        }
    }
    for (std::uint32_t i = 0; i < count; ++i)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily_ = i;
            break;
        }
    }
    if (computeFamily_ == UINT32_MAX) computeFamily_ = graphicsFamily_;
    if (copyFamily_    == UINT32_MAX) copyFamily_    = graphicsFamily_;
}

void VulkanAdapter::QueryFeatures()
{
    // Chain the Vulkan 1.3 + extension feature structs.
    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceVulkan12Features v12{}; v12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features v13{}; v13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f2.pNext = &v12; v12.pNext = &v13;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeat{};
    accelFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeat{};
    rtFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{};
    rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeat{};
    meshFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    v13.pNext = &accelFeat; accelFeat.pNext = &rtFeat;
    rtFeat.pNext = &rayQuery; rayQuery.pNext = &meshFeat;

    vkGetPhysicalDeviceFeatures2(physicalDevice_, &f2);

    features_.Set(Feature::HardwareDevice);
    if (v12.descriptorIndexing)        features_.Set(Feature::DescriptorIndexing);
    if (v12.descriptorIndexing)        features_.Set(Feature::Bindless);
    if (v12.descriptorIndexing)        features_.Set(Feature::BindlessTextureArray);
    if (v12.timelineSemaphore)         features_.Set(Feature::TimelineSemaphore);
    if (v12.bufferDeviceAddress)       features_.Set(Feature::BufferDeviceAddress);
    if (v12.shaderFloat16)             features_.Set(Feature::Float16);
    if (v12.shaderInt8)                features_.Set(Feature::Int8);
    if (f2.features.shaderInt16)       features_.Set(Feature::Int16);
    if (f2.features.shaderInt64)       features_.Set(Feature::Int64);
    if (f2.features.drawIndirectFirstInstance) features_.Set(Feature::MultiDrawIndirect);
    if (v12.drawIndirectCount)         features_.Set(Feature::DrawIndirectCount);
    if (v13.synchronization2)          features_.Set(Feature::EnhancedBarriers);
    if (v13.dynamicRendering)          features_.Set(Feature::DynamicRendering);
    if (accelFeat.accelerationStructure && rtFeat.rayTracingPipeline)
    {
        features_.Set(Feature::RayTracing);
    }
    if (rayQuery.rayQuery)             features_.Set(Feature::RayQuery);
    if (meshFeat.meshShader)           features_.Set(Feature::MeshShader);
    if (meshFeat.taskShader)           features_.Set(Feature::TaskShader);

    features_.Set(Feature::PipelineCache);

    if (isNvidia_)
    {
        features_.Set(Feature::VendorNvApi);
#if defined(HYLUX_CAPTURE_NSIGHT)
        features_.Set(Feature::CaptureNsight);
#endif
#if defined(HYLUX_CRASH_AFTERMATH)
        features_.Set(Feature::CrashReporterNvAftermath);
#endif
    }
    if (desc_.vendorId == kAmdVendorId)
    {
        features_.Set(Feature::VendorAmdAgs);
    }
    else if (desc_.vendorId == kIntelVendorId)
    {
        features_.Set(Feature::VendorIntelXeSdk);
    }
}

Ref<IRHIDevice> VulkanAdapter::CreateDevice(const DeviceDesc& desc)
{
    auto device = MakeRef<VulkanDevice>(this, desc);
    if (!device || !device->IsValid())
    {
        return {};
    }
    return device;
}

RHINativeHandle VulkanAdapter::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPhysicalDevice,
                           reinterpret_cast<std::uint64_t>(physicalDevice_)};
}

} // namespace Hylux::RHI::Vulkan
