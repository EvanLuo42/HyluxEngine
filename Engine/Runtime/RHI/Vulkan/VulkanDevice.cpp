/// @file
/// @brief VulkanDevice implementation. Creates VkDevice with the negotiated feature set,
///        VMA allocator, queue wrappers, and optionally the Aftermath crash reporter.

#include "RHI/Vulkan/VulkanDevice.h"

#include "RHI/Diagnostics/IGpuCrashReporter.h"
#include "RHI/Vulkan/VulkanAdapter.h"
#include "RHI/Vulkan/VulkanCommandPool.h"
#include "RHI/Vulkan/VulkanFence.h"
#include "RHI/Vulkan/VulkanFormat.h"
#include "RHI/Vulkan/VulkanInstance.h"
#include "RHI/Vulkan/VulkanQueue.h"

#if defined(HYLUX_CRASH_AFTERMATH)
#include "RHI/Diagnostics/Aftermath/AftermathDeviceExtensions.h"
#include "RHI/Diagnostics/Aftermath/AftermathGpuCrashReporter.h"
#endif

#include <algorithm>
#include <cstring>
#include <set>

namespace Hylux::RHI::Vulkan
{

namespace
{

bool HasDeviceExtension(const std::vector<VkExtensionProperties>& list, const char* name)
{
    for (const auto& e : list)
    {
        if (std::strcmp(e.extensionName, name) == 0) return true;
    }
    return false;
}

} // namespace

VulkanDevice::VulkanDevice(VulkanAdapter* adapter, const DeviceDesc& desc)
    : adapter_(adapter), desc_(desc)
{
    if (!Initialize())
    {
        Shutdown();
    }
}

VulkanDevice::~VulkanDevice()
{
    Shutdown();
}

bool VulkanDevice::Initialize()
{
    VkPhysicalDevice physicalDevice = adapter_->GetVkPhysicalDevice();

    // Enumerate available device extensions ----------------------------------------------
    std::uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExts(extCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, availableExts.data());

    std::vector<const char*> enabledExts;
    auto tryEnable = [&](const char* name, bool required) {
        if (HasDeviceExtension(availableExts, name)) { enabledExts.push_back(name); return true; }
        if (required)
        {
            HYLUX_LOG(::Hylux::LogRender, Error, "Vulkan: required device extension missing: {}", name);
        }
        return false;
    };

    tryEnable(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
    tryEnable(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, false);
    tryEnable(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, false);
    tryEnable(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, false);
    tryEnable(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, false);
    tryEnable(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, false);

    if (adapter_->IsFeatureSupported(Feature::RayTracing))
    {
        tryEnable(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false);
        tryEnable(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false);
        tryEnable(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, false);
    }
    if (adapter_->IsFeatureSupported(Feature::MeshShader))
    {
        tryEnable(VK_EXT_MESH_SHADER_EXTENSION_NAME, false);
    }

#if defined(HYLUX_CRASH_AFTERMATH)
    const bool wantAftermath =
        (desc_.crashReporter == GpuCrashReporterKind::NvAftermath) ||
        (desc_.crashReporter == GpuCrashReporterKind::Auto &&
         adapter_->IsFeatureSupported(Feature::CrashReporterNvAftermath));
    if (wantAftermath)
    {
        tryEnable(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME, false);
        tryEnable(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME, false);
    }
#else
    const bool wantAftermath = false;
#endif

    // Construct the crash reporter early so it can decorate VkDeviceCreateInfo's pNext.
    Ref<IGpuCrashReporter> reporter;
#if defined(HYLUX_CRASH_AFTERMATH)
    if (wantAftermath)
    {
        auto am = MakeRef<AftermathGpuCrashReporter>();
        if (am && am->IsAvailable())
        {
            reporter = am;
        }
        else if (desc_.crashReporter == GpuCrashReporterKind::NvAftermath)
        {
            HYLUX_LOG(::Hylux::LogRender, Error,
                      "Aftermath requested but EnableGpuCrashDumps failed; reporter disabled.");
        }
    }
#endif

    // Queue create infos -----------------------------------------------------------------
    std::set<std::uint32_t> uniqueFamilies = {adapter_->GetGraphicsQueueFamily()};
    if (desc_.computeQueueCount > 0) uniqueFamilies.insert(adapter_->GetComputeQueueFamily());
    if (desc_.copyQueueCount > 0)    uniqueFamilies.insert(adapter_->GetCopyQueueFamily());

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());
    const float queuePriorities[1] = {1.0f};
    for (std::uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount       = 1;
        qi.pQueuePriorities = queuePriorities;
        queueInfos.push_back(qi);
    }

    // Feature chain ----------------------------------------------------------------------
    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceVulkan11Features v11{}; v11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    VkPhysicalDeviceVulkan12Features v12{}; v12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features v13{}; v13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f2.pNext = &v11; v11.pNext = &v12; v12.pNext = &v13;
    void** chainTail = &v13.pNext;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeat{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rtFeat{};
    VkPhysicalDeviceRayQueryFeaturesKHR              rqFeat{};
    VkPhysicalDeviceMeshShaderFeaturesEXT            meshFeat{};
    if (adapter_->IsFeatureSupported(Feature::RayTracing))
    {
        accelFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rtFeat.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rqFeat.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        *chainTail = &accelFeat; chainTail = &accelFeat.pNext;
        *chainTail = &rtFeat;    chainTail = &rtFeat.pNext;
        *chainTail = &rqFeat;    chainTail = &rqFeat.pNext;
    }
    if (adapter_->IsFeatureSupported(Feature::MeshShader))
    {
        meshFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        *chainTail = &meshFeat; chainTail = &meshFeat.pNext;
    }

    // Re-query to populate per-feature values, then enable everything the adapter offers.
    vkGetPhysicalDeviceFeatures2(physicalDevice, &f2);

    // Mesh-shader sub-features that drag in extensions we have not enabled. Driver may
    // report support, but turning them on without the prerequisite feature/extension is a
    // Vulkan spec violation flagged by VK_LAYER_KHRONOS_validation. Strip them here until
    // the dependent extensions (e.g. VK_KHR_fragment_shading_rate) are wired up.
    if (adapter_->IsFeatureSupported(Feature::MeshShader))
    {
        meshFeat.primitiveFragmentShadingRateMeshShader = VK_FALSE;
    }

#if defined(HYLUX_CRASH_AFTERMATH)
    VkDeviceDiagnosticsConfigCreateInfoNV diagInfo{};
    if (reporter)
    {
        AftermathDecorateDeviceCreateInfo(diagInfo);
        diagInfo.pNext = const_cast<void*>(f2.pNext);
        f2.pNext = &diagInfo;
    }
#endif

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pNext                   = &f2;
    ci.queueCreateInfoCount    = static_cast<std::uint32_t>(queueInfos.size());
    ci.pQueueCreateInfos       = queueInfos.data();
    ci.enabledExtensionCount   = static_cast<std::uint32_t>(enabledExts.size());
    ci.ppEnabledExtensionNames = enabledExts.data();

    HYLUX_VK_RETURN_IF_FAILED(vkCreateDevice(physicalDevice, &ci, nullptr, &device_), false);
    volkLoadDevice(device_);

    hasDebugUtils_ = adapter_->GetInstance()->HasDebugUtils();

    // Build queue wrappers ---------------------------------------------------------------
    auto pullQueue = [&](QueueType type, std::uint32_t family, std::uint32_t count) {
        auto& vec = queues_[static_cast<std::size_t>(type)];
        for (std::uint32_t i = 0; i < count; ++i)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(device_, family, 0, &q);
            vec.push_back(MakeRef<VulkanQueue>(this, type, family, i, q));
        }
    };
    pullQueue(QueueType::Graphics, adapter_->GetGraphicsQueueFamily(), 1);
    if (desc_.computeQueueCount > 0)
    {
        pullQueue(QueueType::Compute, adapter_->GetComputeQueueFamily(), 1);
    }
    if (desc_.copyQueueCount > 0)
    {
        pullQueue(QueueType::Copy, adapter_->GetCopyQueueFamily(), 1);
    }

    // VMA --------------------------------------------------------------------------------
    VmaVulkanFunctions vmaFns{};
    vmaFns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFns.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocCi{};
    allocCi.physicalDevice   = physicalDevice;
    allocCi.device           = device_;
    allocCi.instance         = adapter_->GetInstance()->GetVkInstance();
    allocCi.vulkanApiVersion = VK_API_VERSION_1_3;
    allocCi.pVulkanFunctions = &vmaFns;
    if (v12.bufferDeviceAddress)
    {
        allocCi.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }
    HYLUX_VK_RETURN_IF_FAILED(vmaCreateAllocator(&allocCi, &vmaAllocator_), false);

    // Limits -----------------------------------------------------------------------------
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    limits_.maxBufferSize                  = props.limits.maxStorageBufferRange;
    limits_.maxTexture1DSize               = props.limits.maxImageDimension1D;
    limits_.maxTexture2DSize               = props.limits.maxImageDimension2D;
    limits_.maxTexture3DSize               = props.limits.maxImageDimension3D;
    limits_.maxTextureCubeSize             = props.limits.maxImageDimensionCube;
    limits_.maxTextureArrayLayers          = props.limits.maxImageArrayLayers;
    limits_.maxColorAttachments            = props.limits.maxColorAttachments;
    limits_.maxVertexAttributes            = props.limits.maxVertexInputAttributes;
    limits_.maxComputeWorkgroupCountX      = props.limits.maxComputeWorkGroupCount[0];
    limits_.maxComputeWorkgroupCountY      = props.limits.maxComputeWorkGroupCount[1];
    limits_.maxComputeWorkgroupCountZ      = props.limits.maxComputeWorkGroupCount[2];
    limits_.maxComputeWorkgroupSizeX       = props.limits.maxComputeWorkGroupSize[0];
    limits_.maxComputeWorkgroupSizeY       = props.limits.maxComputeWorkGroupSize[1];
    limits_.maxComputeWorkgroupSizeZ       = props.limits.maxComputeWorkGroupSize[2];
    limits_.maxComputeWorkgroupInvocations = props.limits.maxComputeWorkGroupInvocations;
    limits_.maxPushConstantSize            = props.limits.maxPushConstantsSize;
    limits_.minUniformBufferOffsetAlignment= static_cast<std::uint32_t>(props.limits.minUniformBufferOffsetAlignment);
    limits_.minStorageBufferOffsetAlignment= static_cast<std::uint32_t>(props.limits.minStorageBufferOffsetAlignment);
    limits_.minTexelBufferOffsetAlignment  = static_cast<std::uint32_t>(props.limits.minTexelBufferOffsetAlignment);
    limits_.timestampPeriodNanos           = static_cast<std::uint32_t>(props.limits.timestampPeriod);
    limits_.maxBindlessSrvCbvUav           = desc_.bindlessSizes.srvCbvUavCapacity;
    limits_.maxBindlessSampler             = desc_.bindlessSizes.samplerCapacity;

    // Enabled features set: just mirror the adapter's set with the runtime extension gates.
    enabledFeatures_ = adapter_->GetSupportedFeatures();

    // Finalize crash reporter binding now that VkDevice exists.
#if defined(HYLUX_CRASH_AFTERMATH)
    if (reporter)
    {
        static_cast<AftermathGpuCrashReporter*>(reporter.Get())->BindDevice(device_);
        AttachCrashReporter(reporter);
        HYLUX_LOG(::Hylux::LogRender, Info,
                  "Vulkan: Aftermath crash reporter bound to device.");
    }
#endif

    HYLUX_LOG(::Hylux::LogRender, Info, "VulkanDevice ready (\"{}\")", adapter_->GetDesc().name);
    return true;
}

void VulkanDevice::Shutdown() noexcept
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);
    }
    for (auto& v : queues_) v.clear();
    pipelineCaches_.clear();
    crashReporter_.Reset();
    if (vmaAllocator_)
    {
        vmaDestroyAllocator(vmaAllocator_);
        vmaAllocator_ = nullptr;
    }
    if (device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

IRHIAdapter*  VulkanDevice::GetAdapter()  const noexcept { return adapter_; }
IRHIInstance* VulkanDevice::GetInstance() const noexcept { return adapter_->GetInstance(); }

FormatSupport VulkanDevice::GetFormatSupport(Format format) const noexcept
{
    FormatSupport support{};
    VkFormat vk = ToVkFormat(format);
    if (vk == VK_FORMAT_UNDEFINED) return support;
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(adapter_->GetVkPhysicalDevice(), vk, &props);
    const auto& opt = props.optimalTilingFeatures;
    if (opt & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)         support.bits |= FormatSupport::kSampledImage;
    if (opt & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
        support.bits |= FormatSupport::kSampledImageFilterLinear;
    if (opt & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)         support.bits |= FormatSupport::kStorageImage;
    if (opt & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)  support.bits |= FormatSupport::kStorageImageAtomic;
    if (opt & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)      support.bits |= FormatSupport::kColorAttachment;
    if (opt & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)support.bits |= FormatSupport::kColorAttachmentBlend;
    if (opt & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) support.bits |= FormatSupport::kDepthStencilAttachment;
    if (opt & VK_FORMAT_FEATURE_BLIT_SRC_BIT)              support.bits |= FormatSupport::kBlitSrc;
    if (opt & VK_FORMAT_FEATURE_BLIT_DST_BIT)              support.bits |= FormatSupport::kBlitDst;
    if (props.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
        support.bits |= FormatSupport::kVertexBuffer;
    if (props.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
        support.bits |= FormatSupport::kUniformTexelBuffer;
    if (props.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
        support.bits |= FormatSupport::kStorageTexelBuffer;
    return support;
}

Ref<IRHIQueue> VulkanDevice::GetQueue(QueueType type, std::uint32_t index) const
{
    const auto& v = queues_[static_cast<std::size_t>(type)];
    if (index >= v.size()) return {};
    return v[index];
}

std::uint32_t VulkanDevice::GetQueueCount(QueueType type) const noexcept
{
    return static_cast<std::uint32_t>(queues_[static_cast<std::size_t>(type)].size());
}

Ref<IRHIFence> VulkanDevice::CreateFence(std::uint64_t initialValue)
{
    auto f = MakeRef<VulkanFence>(this, initialValue);
    if (!f || f->GetVkSemaphore() == VK_NULL_HANDLE) return {};
    return f;
}

Ref<IRHICommandPool> VulkanDevice::CreateCommandPool(QueueType type, CommandPoolFlags flags)
{
    auto pool = MakeRef<VulkanCommandPool>(this, type, flags);
    if (!pool || pool->GetVkCommandPool() == VK_NULL_HANDLE) return {};
    return pool;
}

void VulkanDevice::AttachCrashReporter(Ref<IGpuCrashReporter> reporter) noexcept
{
    crashReporter_ = std::move(reporter);
}

IGraphicsCaptureTool* VulkanDevice::GetCaptureTool() const noexcept
{
    return adapter_->GetInstance()->GetCaptureTool();
}

IGpuCrashReporter* VulkanDevice::GetCrashReporter() const noexcept
{
    return crashReporter_.Get();
}

bool VulkanDevice::WaitIdle()
{
    if (device_ == VK_NULL_HANDLE) return false;
    return vkDeviceWaitIdle(device_) == VK_SUCCESS;
}

VkPhysicalDevice VulkanDevice::GetVkPhysicalDevice() const noexcept
{
    return adapter_ ? adapter_->GetVkPhysicalDevice() : VK_NULL_HANDLE;
}

VkInstance VulkanDevice::GetVkInstance() const noexcept
{
    return adapter_ ? adapter_->GetInstance()->GetVkInstance() : VK_NULL_HANDLE;
}

RHINativeHandle VulkanDevice::GetNativeHandle(NativeHandleQuery query) const noexcept
{
    switch (query)
    {
        case NativeHandleQuery::Allocation:
            return RHINativeHandle{RHINativeHandleKind::VmaAllocator,
                                   reinterpret_cast<std::uint64_t>(vmaAllocator_)};
        default:
            return RHINativeHandle{RHINativeHandleKind::VkDevice,
                                   reinterpret_cast<std::uint64_t>(device_)};
    }
}

} // namespace Hylux::RHI::Vulkan
