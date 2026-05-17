/// @file
/// @brief VulkanSampler implementation.

#include "RHI/Vulkan/VulkanSampler.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"

namespace Hylux::RHI::Vulkan
{

VulkanSampler::VulkanSampler(VulkanDevice* device, const SamplerDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    VkSamplerCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter        = ToVkFilter(desc.magFilter);
    ci.minFilter        = ToVkFilter(desc.minFilter);
    ci.mipmapMode       = ToVkMipmapMode(desc.mipFilter);
    ci.addressModeU     = ToVkAddressMode(desc.addressU);
    ci.addressModeV     = ToVkAddressMode(desc.addressV);
    ci.addressModeW     = ToVkAddressMode(desc.addressW);
    ci.mipLodBias       = desc.mipLodBias;
    ci.minLod           = desc.minLod;
    ci.maxLod           = desc.maxLod;
    ci.anisotropyEnable = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    ci.maxAnisotropy    = desc.maxAnisotropy;
    ci.compareEnable    = desc.compareEnable ? VK_TRUE : VK_FALSE;
    ci.compareOp        = ToVkCompareOp(desc.compareOp);
    ci.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    if (vkCreateSampler(device->GetVkDevice(), &ci, nullptr, &sampler_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateSampler failed");
        sampler_ = VK_NULL_HANDLE;
    }
}

VulkanSampler::~VulkanSampler()
{
    if (sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_->GetVkDevice(), sampler_, nullptr);
    }
}

RHINativeHandle VulkanSampler::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkSampler,
                           reinterpret_cast<std::uint64_t>(sampler_)};
}

void VulkanSampler::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_SAMPLER,
                      reinterpret_cast<std::uint64_t>(sampler_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
