/// @file
/// @brief VulkanSurface + VulkanSwapchain implementations.

#include "RHI/Vulkan/VulkanSurface.h"

#include "RHI/Vulkan/VulkanAdapter.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"
#include "RHI/Vulkan/VulkanFence.h"
#include "RHI/Vulkan/VulkanFormat.h"
#include "RHI/Vulkan/VulkanInstance.h"
#include "RHI/Vulkan/VulkanTexture.h"

namespace Hylux::RHI::Vulkan
{

// VulkanSurface ---------------------------------------------------------------------------

VulkanSurface::VulkanSurface(VulkanDevice* device, const PlatformWindowHandle& window)
    : VulkanObject(device), window_(window)
{
    VkInstance instance = device->GetVkInstance();

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (window.system == PlatformWindowSystem::Win32)
    {
        const auto pfnCreate = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
        if (pfnCreate == nullptr)
        {
            HYLUX_LOG(::Hylux::LogRender, Error,
                      "VulkanSurface: vkCreateWin32SurfaceKHR not exported by the Vulkan ICD; "
                      "is VK_KHR_win32_surface enabled on the instance?");
            surface_ = VK_NULL_HANDLE;
            return;
        }
        VkWin32SurfaceCreateInfoKHR ci{};
        ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        ci.hwnd      = static_cast<HWND>(window.nativeWindow);
        ci.hinstance = static_cast<HINSTANCE>(window.nativeDisplay);
        if (pfnCreate(instance, &ci, nullptr, &surface_) != VK_SUCCESS)
        {
            HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateWin32SurfaceKHR failed");
            surface_ = VK_NULL_HANDLE;
        }
        return;
    }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    if (window.system == PlatformWindowSystem::Xcb)
    {
        const auto pfnCreate = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(
            vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
        if (pfnCreate == nullptr)
        {
            HYLUX_LOG(::Hylux::LogRender, Error,
                      "VulkanSurface: vkCreateXcbSurfaceKHR not exported by the Vulkan ICD; "
                      "is VK_KHR_xcb_surface enabled on the instance?");
            surface_ = VK_NULL_HANDLE;
            return;
        }
        VkXcbSurfaceCreateInfoKHR ci{};
        ci.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        ci.connection = static_cast<xcb_connection_t*>(window.nativeDisplay);
        ci.window     = static_cast<xcb_window_t>(reinterpret_cast<std::uintptr_t>(window.nativeWindow));
        if (pfnCreate(instance, &ci, nullptr, &surface_) != VK_SUCCESS)
        {
            HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateXcbSurfaceKHR failed");
            surface_ = VK_NULL_HANDLE;
        }
        return;
    }
#endif

    HYLUX_LOG(::Hylux::LogRender, Error,
              "VulkanSurface: unsupported PlatformWindowSystem={}",
              static_cast<std::uint32_t>(window.system));
}

VulkanSurface::~VulkanSurface()
{
    if (surface_ != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(device_->GetVkInstance(), surface_, nullptr);
}

Extent2D VulkanSurface::GetCurrentExtent() const noexcept
{
    if (surface_ == VK_NULL_HANDLE) return {};
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->GetVkPhysicalDevice(), surface_, &caps);
    return {caps.currentExtent.width, caps.currentExtent.height};
}

RHINativeHandle VulkanSurface::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkSurfaceKHR,
                           reinterpret_cast<std::uint64_t>(surface_)};
}

// VulkanSwapchain -------------------------------------------------------------------------

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, VulkanSurface* surface, const SwapchainDesc& desc)
    : VulkanObject(device), surface_(surface), desc_(desc)
{
    if (!Build(desc.extent))
    {
        Teardown();
    }
}

VulkanSwapchain::~VulkanSwapchain()
{
    Teardown();
}

bool VulkanSwapchain::Build(Extent2D extent)
{
    VkSurfaceKHR vkSurface = surface_->GetVkSurface();
    if (vkSurface == VK_NULL_HANDLE) return false;

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->GetVkPhysicalDevice(), vkSurface, &caps);

    VkExtent2D vkExtent;
    vkExtent.width  = extent.width  > 0 ? extent.width  : caps.currentExtent.width;
    vkExtent.height = extent.height > 0 ? extent.height : caps.currentExtent.height;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = vkSurface;
    ci.minImageCount    = desc_.bufferCount;
    ci.imageFormat      = ToVkFormat(desc_.format);
    ci.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    ci.imageExtent      = vkExtent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = ToVkSwapchainUsage(desc_.usage);
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform     = caps.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = ToVkPresentMode(desc_.presentMode);
    ci.clipped          = VK_TRUE;
    ci.oldSwapchain     = swapchain_;

    VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device_->GetVkDevice(), &ci, nullptr, &newSwapchain) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateSwapchainKHR failed");
        return false;
    }
    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device_->GetVkDevice(), swapchain_, nullptr);
    }
    swapchain_ = newSwapchain;
    desc_.extent = {vkExtent.width, vkExtent.height};

    std::uint32_t count = 0;
    vkGetSwapchainImagesKHR(device_->GetVkDevice(), swapchain_, &count, nullptr);
    images_.resize(count);
    vkGetSwapchainImagesKHR(device_->GetVkDevice(), swapchain_, &count, images_.data());

    backBuffers_.clear();
    backBuffers_.reserve(count);
    TextureDesc td{};
    td.format         = desc_.format;
    td.extent         = {vkExtent.width, vkExtent.height, 1};
    td.usage          = TextureUsage::ColorAttachment;
    td.dimension      = TextureDimension::Tex2D;
    for (VkImage img : images_)
    {
        backBuffers_.push_back(MakeRef<VulkanTexture>(device_, td, img));
    }
    return true;
}

void VulkanSwapchain::Teardown() noexcept
{
    backBuffers_.clear();
    images_.clear();
    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device_->GetVkDevice(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

IRHISurface* VulkanSwapchain::GetSurface() const noexcept { return surface_.Get(); }

IRHITexture* VulkanSwapchain::GetBackBuffer(std::uint32_t index) const noexcept
{
    return index < backBuffers_.size() ? backBuffers_[index].Get() : nullptr;
}

bool VulkanSwapchain::AcquireNextImage(IRHIFence* outSignalFence, std::uint64_t signalValue,
                                       std::uint32_t& outImageIndex)
{
    if (swapchain_ == VK_NULL_HANDLE) return false;
    VkSemaphore sem = outSignalFence
        ? static_cast<VulkanFence*>(outSignalFence)->GetVkSemaphore()
        : VK_NULL_HANDLE;
    const VkResult r = vkAcquireNextImageKHR(device_->GetVkDevice(), swapchain_, UINT64_MAX,
                                             sem, VK_NULL_HANDLE, &outImageIndex);
    if (sem && r == VK_SUCCESS)
    {
        // Signal the timeline semaphore to the caller-supplied value via a CPU-side host signal.
        outSignalFence->Signal(signalValue);
    }
    return r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR;
}

bool VulkanSwapchain::Resize(Extent2D newExtent)
{
    vkDeviceWaitIdle(device_->GetVkDevice());
    return Build(newExtent);
}

RHINativeHandle VulkanSwapchain::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkSwapchainKHR,
                           reinterpret_cast<std::uint64_t>(swapchain_)};
}

} // namespace Hylux::RHI::Vulkan
