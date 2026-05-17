/// @file
/// @brief Vulkan implementations of IRHITexture and IRHITextureView.

#pragma once

#include "RHI/IRHITexture.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <vk_mem_alloc.h>

namespace Hylux::RHI::Vulkan
{

class VulkanTexture final : public VulkanObject, public IRHITexture
{
public:
    /// @brief Constructs a new image owned by this texture (CreateTexture path).
    VulkanTexture(VulkanDevice* device, const TextureDesc& desc);

    /// @brief Constructs a wrapper around an externally-owned image (swapchain back-buffer path).
    VulkanTexture(VulkanDevice* device, const TextureDesc& desc, VkImage externalImage);

    ~VulkanTexture() override;

    [[nodiscard]] bool IsValid() const noexcept { return image_ != VK_NULL_HANDLE; }

    [[nodiscard]] const TextureDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] BindlessIndex GetBindlessIndex(BindlessKind /*kind*/) const noexcept override
    {
        return BindlessIndex::Invalid;
    }

    [[nodiscard]] VkImage       GetVkImage()       const noexcept { return image_; }
    [[nodiscard]] VmaAllocation GetVmaAllocation() const noexcept { return allocation_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    TextureDesc   desc_;
    VkImage       image_{VK_NULL_HANDLE};
    VmaAllocation allocation_{nullptr};
    bool          ownsImage_{true};
};

class VulkanTextureView final : public VulkanObject, public IRHITextureView
{
public:
    VulkanTextureView(VulkanDevice* device, VulkanTexture* parent, const TextureViewDesc& desc);
    ~VulkanTextureView() override;

    [[nodiscard]] bool IsValid() const noexcept { return view_ != VK_NULL_HANDLE; }

    [[nodiscard]] const TextureViewDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] IRHITexture* GetTexture() const noexcept override;
    [[nodiscard]] BindlessIndex GetBindlessIndex(BindlessKind /*kind*/) const noexcept override
    {
        return BindlessIndex::Invalid;
    }

    [[nodiscard]] VkImageView GetVkImageView() const noexcept { return view_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    Ref<VulkanTexture> parent_;
    TextureViewDesc    desc_;
    VkImageView        view_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
