/// @file
/// @brief Descriptor-set / layout implementations (skeletal — explicit descriptor sets
///        are an edge-case path in the bindless-first engine).

#include "RHI/Vulkan/VulkanDescriptorSet.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"

#include <atomic>

namespace Hylux::RHI::Vulkan
{

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device,
                                                     const DescriptorSetLayoutDesc& desc)
    : VulkanObject(device)
{
    bindingsCopy_.assign(desc.bindings.begin(), desc.bindings.end());
    desc_.bindings = bindingsCopy_;

    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    vkBindings.reserve(bindingsCopy_.size());
    for (const auto& b : bindingsCopy_)
    {
        VkDescriptorSetLayoutBinding vb{};
        vb.binding         = b.binding;
        vb.descriptorType  = ToVkDescriptorType(b.type);
        vb.descriptorCount = b.count;
        vb.stageFlags      = ToVkShaderStageFlags(b.stages);
        vkBindings.push_back(vb);
    }

    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = static_cast<std::uint32_t>(vkBindings.size());
    ci.pBindings    = vkBindings.data();
    if (vkCreateDescriptorSetLayout(device->GetVkDevice(), &ci, nullptr, &layout_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateDescriptorSetLayout failed");
        layout_ = VK_NULL_HANDLE;
    }
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    if (layout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_->GetVkDevice(), layout_, nullptr);
}

RHINativeHandle VulkanDescriptorSetLayout::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkDescriptorSetLayout,
                           reinterpret_cast<std::uint64_t>(layout_)};
}

void VulkanDescriptorSetLayout::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                      reinterpret_cast<std::uint64_t>(layout_), debugName_);
}

// VulkanDescriptorSet ---------------------------------------------------------------------

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice* device, VulkanDescriptorSetLayout* layout,
                                         VkDescriptorPool pool, VkDescriptorSet set)
    : VulkanObject(device), layout_(layout), pool_(pool), set_(set) {}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
    // Sets are freed implicitly when the pool is reset / destroyed.
}

IRHIDescriptorSetLayout* VulkanDescriptorSet::GetLayout() const noexcept { return layout_.Get(); }

void VulkanDescriptorSet::Update(std::span<const DescriptorWrite> /*writes*/)
{
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed))
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "VulkanDescriptorSet::Update not implemented; descriptor writes dropped "
                  "(bindless-first engine — explicit descriptor sets are a stub path)");
    }
}

RHINativeHandle VulkanDescriptorSet::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkDescriptorSet,
                           reinterpret_cast<std::uint64_t>(set_)};
}

void VulkanDescriptorSet::OnDebugNameChanged() {}

} // namespace Hylux::RHI::Vulkan
