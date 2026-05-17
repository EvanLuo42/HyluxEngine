/// @file
/// @brief Vulkan implementations of IRHISurface and IRHISwapchain.

#pragma once

#include "RHI/IRHISurface.h"
#include "RHI/Vulkan/VulkanObject.h"
#include "RHI/Vulkan/VulkanTexture.h"

#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanSurface final : public VulkanObject, public IRHISurface
{
public:
    VulkanSurface(VulkanDevice* device, const PlatformWindowHandle& window);
    ~VulkanSurface() override;

    [[nodiscard]] bool IsValid() const noexcept { return surface_ != VK_NULL_HANDLE; }

    [[nodiscard]] const PlatformWindowHandle& GetWindowHandle() const noexcept override { return window_; }
    [[nodiscard]] Extent2D GetCurrentExtent() const noexcept override;

    [[nodiscard]] VkSurfaceKHR GetVkSurface() const noexcept { return surface_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override {}

private:
    PlatformWindowHandle window_;
    VkSurfaceKHR         surface_{VK_NULL_HANDLE};
};

class VulkanSwapchain final : public VulkanObject, public IRHISwapchain
{
public:
    VulkanSwapchain(VulkanDevice* device, VulkanSurface* surface, const SwapchainDesc& desc);
    ~VulkanSwapchain() override;

    [[nodiscard]] bool IsValid() const noexcept { return swapchain_ != VK_NULL_HANDLE; }

    [[nodiscard]] const SwapchainDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] IRHISurface* GetSurface() const noexcept override;
    [[nodiscard]] IRHITexture* GetBackBuffer(std::uint32_t index) const noexcept override;
    [[nodiscard]] std::uint32_t GetBackBufferCount() const noexcept override
    {
        return static_cast<std::uint32_t>(images_.size());
    }

    bool AcquireNextImage(IRHIFence* outSignalFence, std::uint64_t signalValue,
                          std::uint32_t& outImageIndex) override;
    bool Resize(Extent2D newExtent) override;

    [[nodiscard]] VkSwapchainKHR GetVkSwapchain() const noexcept { return swapchain_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override {}

private:
    bool Build(Extent2D extent);
    void Teardown() noexcept;

    Ref<VulkanSurface>                surface_;
    SwapchainDesc                     desc_;
    VkSwapchainKHR                    swapchain_{VK_NULL_HANDLE};
    std::vector<VkImage>              images_;
    std::vector<Ref<VulkanTexture>>   backBuffers_;
};

} // namespace Hylux::RHI::Vulkan
