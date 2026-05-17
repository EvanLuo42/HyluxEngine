/// @file
/// @brief Sole definition of Hylux::RHI::CreateRHIInstance for the Vulkan backend.
///        Linking this TU into a target satisfies the declaration in CreateRHIInstance.h
///        and binds the engine to the Vulkan backend.

#include "RHI/CreateRHIInstance.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "RHI/Vulkan/VulkanInstance.h"

namespace Hylux::RHI
{

Ref<IRHIInstance> CreateRHIInstance(const InstanceDesc& instanceDesc)
{
    if (instanceDesc.preferredDevice != DeviceType::Vulkan &&
        instanceDesc.preferredDevice != DeviceType::Null)
    {
        HYLUX_LOG(::Hylux::LogRender, Fatal,
                  "CreateRHIInstance: only DeviceType::Vulkan is linked, requested={}",
                  static_cast<std::uint32_t>(instanceDesc.preferredDevice));
        return {};
    }

    auto instance = MakeRef<Vulkan::VulkanInstance>(instanceDesc);
    if (!instance || !instance->IsValid())
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "CreateRHIInstance: VulkanInstance construction failed");
        return {};
    }
    return instance;
}

} // namespace Hylux::RHI
