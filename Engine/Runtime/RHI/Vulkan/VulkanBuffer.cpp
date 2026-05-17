/// @file
/// @brief VulkanBuffer implementation using VMA.

#include "RHI/Vulkan/VulkanBuffer.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"

namespace Hylux::RHI::Vulkan
{

namespace
{

VmaMemoryUsage ToVmaUsage(MemoryUsage usage)
{
    switch (usage)
    {
        case MemoryUsage::GpuOnly:       return VMA_MEMORY_USAGE_GPU_ONLY;
        case MemoryUsage::CpuOnly:       return VMA_MEMORY_USAGE_CPU_ONLY;
        case MemoryUsage::CpuToGpu:      return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case MemoryUsage::GpuToCpu:      return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case MemoryUsage::CpuGpuShared:  return VMA_MEMORY_USAGE_CPU_COPY;
    }
    return VMA_MEMORY_USAGE_GPU_ONLY;
}

} // namespace

VulkanBuffer::VulkanBuffer(VulkanDevice* device, const BufferDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    VkBufferCreateInfo ci{};
    ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size        = desc.size;
    ci.usage       = ToVkBufferUsage(desc.usage);
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci{};
    aci.usage = ToVmaUsage(desc.memory);
    if (desc.memory == MemoryUsage::CpuToGpu || desc.memory == MemoryUsage::CpuOnly ||
        desc.memory == MemoryUsage::CpuGpuShared)
    {
        aci.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    else if (desc.memory == MemoryUsage::GpuToCpu)
    {
        aci.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }

    if (vmaCreateBuffer(device->GetVmaAllocator(), &ci, &aci,
                        &buffer_, &allocation_, nullptr) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vmaCreateBuffer failed (size={})", desc.size);
        buffer_ = VK_NULL_HANDLE;
        return;
    }

    if ((static_cast<std::uint32_t>(desc.usage) &
         static_cast<std::uint32_t>(BufferUsage::ShaderDeviceAddress)) != 0)
    {
        VkBufferDeviceAddressInfo ai{};
        ai.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        ai.buffer = buffer_;
        gpuAddress_ = vkGetBufferDeviceAddress(device->GetVkDevice(), &ai);
    }
}

VulkanBuffer::~VulkanBuffer()
{
    if (buffer_ != VK_NULL_HANDLE)
    {
        if (mappedPtr_) vmaUnmapMemory(device_->GetVmaAllocator(), allocation_);
        vmaDestroyBuffer(device_->GetVmaAllocator(), buffer_, allocation_);
    }
}

void* VulkanBuffer::Map(std::uint64_t /*offset*/, std::uint64_t /*size*/)
{
    if (mappedPtr_) return mappedPtr_;
    if (vmaMapMemory(device_->GetVmaAllocator(), allocation_, &mappedPtr_) != VK_SUCCESS)
    {
        mappedPtr_ = nullptr;
    }
    return mappedPtr_;
}

void VulkanBuffer::Unmap()
{
    if (mappedPtr_)
    {
        vmaUnmapMemory(device_->GetVmaAllocator(), allocation_);
        mappedPtr_ = nullptr;
    }
}

RHINativeHandle VulkanBuffer::GetNativeHandle(NativeHandleQuery query) const noexcept
{
    if (query == NativeHandleQuery::Allocation)
    {
        return RHINativeHandle{RHINativeHandleKind::VmaAllocation,
                               reinterpret_cast<std::uint64_t>(allocation_)};
    }
    return RHINativeHandle{RHINativeHandleKind::VkBuffer,
                           reinterpret_cast<std::uint64_t>(buffer_)};
}

void VulkanBuffer::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_BUFFER,
                      reinterpret_cast<std::uint64_t>(buffer_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
