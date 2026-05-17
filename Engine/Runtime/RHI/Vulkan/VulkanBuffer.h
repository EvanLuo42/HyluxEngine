/// @file
/// @brief Vulkan implementation of IRHIBuffer backed by VMA.

#pragma once

#include "RHI/IRHIBuffer.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <vk_mem_alloc.h>

namespace Hylux::RHI::Vulkan
{

class VulkanBuffer final : public VulkanObject, public IRHIBuffer
{
public:
    VulkanBuffer(VulkanDevice* device, const BufferDesc& desc);
    ~VulkanBuffer() override;

    [[nodiscard]] bool IsValid() const noexcept { return buffer_ != VK_NULL_HANDLE; }

    [[nodiscard]] const BufferDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] std::uint64_t GetSize() const noexcept override { return desc_.size; }
    [[nodiscard]] std::uint64_t GetGpuAddress() const noexcept override { return gpuAddress_; }

    void* Map(std::uint64_t offset = 0, std::uint64_t size = kWholeSize) override;
    void  Unmap() override;

    [[nodiscard]] BindlessIndex GetBindlessIndex(BindlessKind /*kind*/) const noexcept override
    {
        return BindlessIndex::Invalid;  // Bindless registration done by VulkanBindlessHeap (future).
    }

    [[nodiscard]] VkBuffer      GetVkBuffer()      const noexcept { return buffer_; }
    [[nodiscard]] VmaAllocation GetVmaAllocation() const noexcept { return allocation_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    BufferDesc    desc_;
    VkBuffer      buffer_{VK_NULL_HANDLE};
    VmaAllocation allocation_{nullptr};
    std::uint64_t gpuAddress_{0};
    void*         mappedPtr_{nullptr};
};

} // namespace Hylux::RHI::Vulkan
