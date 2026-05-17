/// @file
/// @brief VulkanTexture + VulkanTextureView implementations.

#include "RHI/Vulkan/VulkanTexture.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"
#include "RHI/Vulkan/VulkanFormat.h"

namespace Hylux::RHI::Vulkan
{

VulkanTexture::VulkanTexture(VulkanDevice* device, const TextureDesc& desc)
    : VulkanObject(device), desc_(desc), ownsImage_(true)
{
    VkImageCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = ToVkImageType(desc.dimension);
    ci.format        = ToVkFormat(desc.format);
    ci.extent        = {desc.extent.width, desc.extent.height, desc.extent.depth};
    ci.mipLevels     = desc.mipLevels;
    ci.arrayLayers   = desc.arrayLayers;
    ci.samples       = ToVkSampleCount(desc.sampleCount);
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = ToVkImageUsage(desc.usage);
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (desc.dimension == TextureDimension::TexCube ||
        desc.dimension == TextureDimension::TexCubeArray)
    {
        ci.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (vmaCreateImage(device->GetVmaAllocator(), &ci, &aci,
                       &image_, &allocation_, nullptr) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vmaCreateImage failed");
        image_ = VK_NULL_HANDLE;
    }
}

VulkanTexture::VulkanTexture(VulkanDevice* device, const TextureDesc& desc, VkImage externalImage)
    : VulkanObject(device), desc_(desc), image_(externalImage), ownsImage_(false) {}

VulkanTexture::~VulkanTexture()
{
    if (ownsImage_ && image_ != VK_NULL_HANDLE)
    {
        vmaDestroyImage(device_->GetVmaAllocator(), image_, allocation_);
    }
}

RHINativeHandle VulkanTexture::GetNativeHandle(NativeHandleQuery query) const noexcept
{
    if (query == NativeHandleQuery::Allocation)
    {
        return RHINativeHandle{RHINativeHandleKind::VmaAllocation,
                               reinterpret_cast<std::uint64_t>(allocation_)};
    }
    return RHINativeHandle{RHINativeHandleKind::VkImage,
                           reinterpret_cast<std::uint64_t>(image_)};
}

void VulkanTexture::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_IMAGE,
                      reinterpret_cast<std::uint64_t>(image_), debugName_);
    }
}

// VulkanTextureView -----------------------------------------------------------------------

VulkanTextureView::VulkanTextureView(VulkanDevice* device, VulkanTexture* parent,
                                     const TextureViewDesc& desc)
    : VulkanObject(device), parent_(parent), desc_(desc)
{
    const TextureDesc& td = parent->GetDesc();
    VkImageViewCreateInfo ci{};
    ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image                           = parent->GetVkImage();
    ci.viewType                        = ToVkImageViewType(desc.dimension);
    ci.format                          = ToVkFormat(desc.format != Format::Unknown ? desc.format : td.format);
    ci.subresourceRange.aspectMask     = FormatAspect(td.format);
    ci.subresourceRange.baseMipLevel   = desc.range.baseMipLevel;
    ci.subresourceRange.levelCount     = desc.range.mipLevelCount;
    ci.subresourceRange.baseArrayLayer = desc.range.baseArrayLayer;
    ci.subresourceRange.layerCount     = desc.range.arrayLayerCount;
    if (vkCreateImageView(device->GetVkDevice(), &ci, nullptr, &view_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateImageView failed");
        view_ = VK_NULL_HANDLE;
    }
}

VulkanTextureView::~VulkanTextureView()
{
    if (view_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device_->GetVkDevice(), view_, nullptr);
    }
}

IRHITexture* VulkanTextureView::GetTexture() const noexcept { return parent_.Get(); }

RHINativeHandle VulkanTextureView::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkImageView,
                           reinterpret_cast<std::uint64_t>(view_)};
}

void VulkanTextureView::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_IMAGE_VIEW,
                      reinterpret_cast<std::uint64_t>(view_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
