/// @file
/// @brief Vulkan implementation of IRHIShaderModule. Owns a VkShaderModule and a copy of
///        the SPIR-V bytecode (so the Aftermath crash reporter can register it for symbol
///        resolution at dump time).

#pragma once

#include "RHI/IRHIShaderModule.h"
#include "RHI/Vulkan/VulkanObject.h"

#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanShaderModule final : public VulkanObject, public IRHIShaderModule
{
public:
    VulkanShaderModule(VulkanDevice* device, const ShaderBytecode& bytecode);
    ~VulkanShaderModule() override;

    [[nodiscard]] bool IsValid() const noexcept { return module_ != VK_NULL_HANDLE; }

    [[nodiscard]] ShaderBytecodeFormat GetFormat() const noexcept override { return format_; }
    [[nodiscard]] ShaderStage          GetStage() const noexcept override { return stage_; }
    [[nodiscard]] std::string_view     GetEntryPoint() const noexcept override { return entryPoint_; }
    [[nodiscard]] std::span<const std::byte> GetBytecode() const noexcept override
    {
        return {bytecode_.data(), bytecode_.size()};
    }

    [[nodiscard]] std::uint64_t GetBytecodeHash() const noexcept { return bytecodeHash_; }
    [[nodiscard]] VkShaderModule GetVkShaderModule() const noexcept { return module_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    ShaderBytecodeFormat       format_;
    ShaderStage                stage_;
    std::string                entryPoint_;
    std::vector<std::byte>     bytecode_;
    std::uint64_t              bytecodeHash_{0};
    VkShaderModule             module_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
