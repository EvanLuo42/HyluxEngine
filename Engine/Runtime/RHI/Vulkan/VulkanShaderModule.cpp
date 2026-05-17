/// @file
/// @brief VulkanShaderModule implementation. Computes a 64-bit bytecode hash and
///        registers the shader with the Aftermath crash reporter when one is bound,
///        so crash dumps can resolve faulting shader hashes back to bytecode.

#include "RHI/Vulkan/VulkanShaderModule.h"

#include "RHI/Diagnostics/IGpuCrashReporter.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"

#include <cstring>

namespace Hylux::RHI::Vulkan
{

namespace
{

/// @brief 64-bit FNV-1a hash. Cheap and good enough for the per-frame shader-hash collision
///        space (a few thousand shaders).
std::uint64_t HashBytecode(const void* data, std::size_t size) noexcept
{
    std::uint64_t h = 1469598103934665603ull;
    const auto* p = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

} // namespace

VulkanShaderModule::VulkanShaderModule(VulkanDevice* device, const ShaderBytecode& bc)
    : VulkanObject(device),
      format_(bc.format), stage_(bc.stage), entryPoint_(bc.entryPoint)
{
    bytecode_.assign(bc.data.begin(), bc.data.end());
    bytecodeHash_ = HashBytecode(bytecode_.data(), bytecode_.size());

    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = bytecode_.size();
    ci.pCode    = reinterpret_cast<const std::uint32_t*>(bytecode_.data());
    if (vkCreateShaderModule(device->GetVkDevice(), &ci, nullptr, &module_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateShaderModule failed");
        module_ = VK_NULL_HANDLE;
        return;
    }

    // Auto-register with the crash reporter so the dump can map back to bytecode.
    if (IGpuCrashReporter* reporter = device->GetCrashReporter())
    {
        ShaderBinaryRegistration reg{};
        reg.shaderHash   = bytecodeHash_;
        reg.format       = format_;
        reg.stage        = stage_;
        reg.bytecode     = bytecode_.data();
        reg.bytecodeSize = bytecode_.size();
        reg.debugName    = bc.entryPoint;
        reporter->RegisterShader(reg);
    }
}

VulkanShaderModule::~VulkanShaderModule()
{
    if (module_ != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device_->GetVkDevice(), module_, nullptr);
    }
}

RHINativeHandle VulkanShaderModule::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::None,
                           reinterpret_cast<std::uint64_t>(module_)};
}

void VulkanShaderModule::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_SHADER_MODULE,
                      reinterpret_cast<std::uint64_t>(module_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
