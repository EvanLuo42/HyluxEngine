/// @file
/// @brief Vulkan implementation of IRHIVendorExtension. Stub container for vendor SDK
///        handles (NVAPI, AGS, Intel Xe SDK); IRHIDevice::QueryVendorExtension returns
///        nullptr until a concrete SDK is wired in.

#pragma once

#include "RHI/IRHIVendorExtension.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanVendorExtension final : public VulkanObject, public IRHIVendorExtension
{
public:
    VulkanVendorExtension(VulkanDevice* device, VendorExtensionKind kind, void* nativeCtx);
    ~VulkanVendorExtension() override = default;

    [[nodiscard]] VendorExtensionKind GetKind() const noexcept override { return kind_; }
    [[nodiscard]] std::uint32_t       GetSdkVersion() const noexcept override { return 0; }
    [[nodiscard]] void*               GetNativeContext() const noexcept override { return nativeCtx_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery /*query*/) const noexcept override
    {
        return RHINativeHandle{};
    }
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

private:
    VendorExtensionKind kind_;
    void*               nativeCtx_;
};

} // namespace Hylux::RHI::Vulkan
