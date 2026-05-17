/// @file
/// @brief Vulkan implementation of IRHIAccelerationStructure (stub — full RT pipeline is
///        future work, but the type exists so VulkanDevice::CreateBlas / CreateTlas link).

#pragma once

#include "RHI/IRHIAccelerationStructure.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanAccelerationStructure final : public VulkanObject, public IRHIAccelerationStructure
{
public:
    explicit VulkanAccelerationStructure(VulkanDevice* device) : VulkanObject(device) {}
    ~VulkanAccelerationStructure() override = default;

    [[nodiscard]] std::uint64_t GetGpuAddress()      const noexcept override { return 0; }
    [[nodiscard]] std::uint64_t GetSize()            const noexcept override { return 0; }
    [[nodiscard]] BindlessIndex GetBindlessIndex()   const noexcept override
    {
        return BindlessIndex::Invalid;
    }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery /*query*/) const noexcept override
    {
        return RHINativeHandle{RHINativeHandleKind::VkAccelerationStructureKHR, 0};
    }
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }
};

} // namespace Hylux::RHI::Vulkan
