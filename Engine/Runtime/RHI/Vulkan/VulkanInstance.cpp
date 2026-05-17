/// @file
/// @brief VulkanInstance implementation. Bootstraps Vulkan via volk, enumerates adapters,
///        installs the debug-utils messenger, and optionally loads the configured capture tool.

#include "RHI/Vulkan/VulkanInstance.h"

#include "RHI/Vulkan/VulkanAdapter.h"
#include "RHI/Vulkan/VulkanValidationLayer.h"

#if defined(HYLUX_CAPTURE_NSIGHT)
#include "RHI/Capture/Nsight/NsightGraphicsCaptureTool.h"
#endif

#include <cstring>

namespace Hylux::RHI::Vulkan
{

namespace
{

bool HasInstanceExtension(const std::vector<VkExtensionProperties>& exts, const char* name) noexcept
{
    for (const auto& e : exts)
    {
        if (std::strcmp(e.extensionName, name) == 0) return true;
    }
    return false;
}

} // namespace

VulkanInstance::VulkanInstance(const InstanceDesc& desc)
    : desc_(desc)
{
    if (!Initialize())
    {
        Shutdown();
    }
}

VulkanInstance::~VulkanInstance()
{
    Shutdown();
}

bool VulkanInstance::Initialize()
{
    const VkResult volkInit = volkInitialize();
    if (volkInit != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Fatal,
                  "Vulkan: volkInitialize failed ({}) - no Vulkan ICD installed?",
                  VulkanResultToString(volkInit));
        return false;
    }

    const std::uint32_t loaderVersion = volkGetInstanceVersion();
    HYLUX_LOG(::Hylux::LogRender, Info, "Vulkan loader version {}.{}.{}",
              VK_VERSION_MAJOR(loaderVersion), VK_VERSION_MINOR(loaderVersion),
              VK_VERSION_PATCH(loaderVersion));

    // Enumerate instance extensions ------------------------------------------------------
    std::uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExts(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, availableExts.data());

    std::vector<const char*> enabledExts;
    enabledExts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    enabledExts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    enabledExts.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

    if (HasInstanceExtension(availableExts, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        enabledExts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    hasDebugUtils_ = HasInstanceExtension(availableExts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (hasDebugUtils_)
    {
        enabledExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Layers -----------------------------------------------------------------------------
    std::vector<const char*> enabledLayers;
    const bool wantValidation = desc_.gapiValidation != GapiValidationLevel::Off;
    if (wantValidation)
    {
        if (IsValidationLayerAvailable())
        {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
        else
        {
            HYLUX_LOG(::Hylux::LogRender, Warn,
                      "Vulkan: GAPI validation requested but VK_LAYER_KHRONOS_validation "
                      "is not available (LunarG SDK runtime not installed?). Continuing "
                      "without validation.");
        }
    }

    // Application info -------------------------------------------------------------------
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = desc_.applicationName.empty() ? "Hylux App" : desc_.applicationName.c_str();
    appInfo.applicationVersion = desc_.applicationVersion;
    appInfo.pEngineName        = desc_.engineName.empty() ? "HyluxEngine" : desc_.engineName.c_str();
    appInfo.engineVersion      = desc_.engineVersion;
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    // Validation features chain -----------------------------------------------------------
    VkValidationFeaturesEXT valFeatures{};
    VkValidationFeatureEnableEXT enables[4];
    std::uint32_t enableCount = 0;
    if (wantValidation && desc_.gapiValidation >= GapiValidationLevel::Standard)
    {
        enables[enableCount++] = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
        enables[enableCount++] = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    }
    if (wantValidation && desc_.gapiValidation >= GapiValidationLevel::Gpu)
    {
        enables[enableCount++] = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;
    }
    if (wantValidation && desc_.gapiValidation >= GapiValidationLevel::GpuAggressive)
    {
        enables[enableCount++] = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT;
    }
    valFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    valFeatures.enabledValidationFeatureCount = enableCount;
    valFeatures.pEnabledValidationFeatures    = enables;

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &appInfo;
    ci.enabledLayerCount       = static_cast<std::uint32_t>(enabledLayers.size());
    ci.ppEnabledLayerNames     = enabledLayers.data();
    ci.enabledExtensionCount   = static_cast<std::uint32_t>(enabledExts.size());
    ci.ppEnabledExtensionNames = enabledExts.data();
    if (enableCount > 0)
    {
        ci.pNext = &valFeatures;
    }

    HYLUX_VK_RETURN_IF_FAILED(vkCreateInstance(&ci, nullptr, &instance_), false);
    volkLoadInstance(instance_);

    if (hasDebugUtils_)
    {
        debugMessenger_ = CreateDebugUtilsMessenger(instance_, desc_.gapiValidation);
    }

    // Enumerate physical devices ---------------------------------------------------------
    std::uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "Vulkan: no physical devices found.");
        return false;
    }
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data());
    adapters_.reserve(deviceCount);
    for (VkPhysicalDevice pd : physicalDevices)
    {
        adapters_.push_back(MakeRef<VulkanAdapter>(this, pd));
    }

    // Capture tool ------------------------------------------------------------------------
#if defined(HYLUX_CAPTURE_NSIGHT)
    if (desc_.captureTool == CaptureToolKind::Auto ||
        desc_.captureTool == CaptureToolKind::Nsight)
    {
        auto nsight = MakeRef<NsightGraphicsCaptureTool>(instance_);
        if (nsight && nsight->IsAvailable())
        {
            captureTool_ = nsight;
            HYLUX_LOG(::Hylux::LogRender, Info,
                      "Vulkan: Nsight Graphics capture tool loaded (version {})",
                      nsight->GetSdkVersion());
        }
        else if (desc_.captureTool == CaptureToolKind::Nsight)
        {
            HYLUX_LOG(::Hylux::LogRender, Warn,
                      "Vulkan: Nsight requested but NGFX initialization failed; "
                      "GetCaptureTool() will return null.");
        }
    }
#else
    if (desc_.captureTool != CaptureToolKind::Auto && desc_.captureTool != CaptureToolKind::None)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "Vulkan: captureTool={} requested but no capture SDK is compiled in.",
                  static_cast<std::uint32_t>(desc_.captureTool));
    }
#endif

    HYLUX_LOG(::Hylux::LogRender, Info, "VulkanInstance ready ({} adapter(s))",
              adapters_.size());
    return true;
}

void VulkanInstance::Shutdown() noexcept
{
    adapters_.clear();
    captureTool_.Reset();
    if (debugMessenger_ != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessenger(instance_, debugMessenger_);
        debugMessenger_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

std::uint32_t VulkanInstance::GetAdapterCount() const noexcept
{
    return static_cast<std::uint32_t>(adapters_.size());
}

Ref<IRHIAdapter> VulkanInstance::GetAdapter(std::uint32_t index) const
{
    if (index >= adapters_.size()) return {};
    return adapters_[index];
}

Ref<IRHIAdapter> VulkanInstance::GetDefaultAdapter() const
{
    Ref<IRHIAdapter> integrated;
    Ref<IRHIAdapter> software;
    for (const auto& a : adapters_)
    {
        const AdapterDesc& d = a->GetDesc();
        if (static_cast<std::uint32_t>(d.flags) & static_cast<std::uint32_t>(AdapterFlags::Discrete))
        {
            return a;
        }
        if (!integrated && (static_cast<std::uint32_t>(d.flags) & static_cast<std::uint32_t>(AdapterFlags::Integrated)))
        {
            integrated = a;
        }
        if (!software && (static_cast<std::uint32_t>(d.flags) & static_cast<std::uint32_t>(AdapterFlags::Software)))
        {
            software = a;
        }
    }
    if (integrated) return integrated;
    if (software)   return software;
    return adapters_.empty() ? Ref<IRHIAdapter>{} : Ref<IRHIAdapter>{adapters_.front()};
}

RHINativeHandle VulkanInstance::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkInstance,
                           reinterpret_cast<std::uint64_t>(instance_)};
}

} // namespace Hylux::RHI::Vulkan
