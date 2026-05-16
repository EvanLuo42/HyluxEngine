/// @file
/// @brief RHISubsystem implementation. Brings up the RHI stack via the backend-provided
///        CreateRHIInstance factory and the validation wrapper factory.

#include "RHI/RHISubsystem.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/CreateRHIInstance.h"
#include "RHI/Validation/RhiValidationLayer.h"

#include <utility>

namespace Hylux::RHI
{

RHISubsystem::RHISubsystem(InstanceDesc instanceDesc, DeviceDesc deviceDesc)
    : instanceDesc_(std::move(instanceDesc)),
      deviceDesc_(std::move(deviceDesc))
{
}

void RHISubsystem::Initialize(Engine& /*engine*/)
{
    HYLUX_LOG(LogRender, Info,
              "RHISubsystem::Initialize preferredDevice={} rhiValidation={} gapiValidation={}",
              static_cast<std::uint32_t>(instanceDesc_.preferredDevice),
              static_cast<std::uint32_t>(instanceDesc_.rhiValidation),
              static_cast<std::uint32_t>(instanceDesc_.gapiValidation));

    instance_ = CreateRHIInstance(instanceDesc_);
    if (!instance_)
    {
        HYLUX_LOG(LogRender, Error, "RHISubsystem: CreateRHIInstance returned null");
        return;
    }

    adapter_ = instance_->GetDefaultAdapter();
    if (!adapter_)
    {
        HYLUX_LOG(LogRender, Error, "RHISubsystem: no default adapter available");
        instance_.Reset();
        return;
    }

    HYLUX_LOG(LogRender, Info, "RHISubsystem: selected adapter \"{}\"",
              adapter_->GetDesc().name);

    Ref<IRHIDevice> innerDevice = adapter_->CreateDevice(deviceDesc_);
    if (!innerDevice)
    {
        HYLUX_LOG(LogRender, Error, "RHISubsystem: IRHIAdapter::CreateDevice returned null");
        adapter_.Reset();
        instance_.Reset();
        return;
    }

    if (deviceDesc_.rhiValidation != RhiValidationLevel::Off)
    {
        HYLUX_LOG(LogRender, Info, "RHISubsystem: wrapping device with RHI validation layer level={}",
                  static_cast<std::uint32_t>(deviceDesc_.rhiValidation));
        device_ = CreateRhiValidationWrapper(std::move(innerDevice), deviceDesc_.rhiValidation);
    }
    else
    {
        device_ = std::move(innerDevice);
    }
}

void RHISubsystem::Shutdown()
{
    if (device_)
    {
        HYLUX_LOG(LogRender, Info, "RHISubsystem::Shutdown waiting for device idle");
        device_->WaitIdle();
    }
    device_.Reset();
    adapter_.Reset();
    instance_.Reset();
}

std::vector<TypeId> RHISubsystem::GetDependencies() const
{
    return {};
}

IGraphicsCaptureTool* RHISubsystem::GetCaptureTool() const noexcept
{
    if (device_)
    {
        return device_->GetCaptureTool();
    }
    if (instance_)
    {
        return instance_->GetCaptureTool();
    }
    return nullptr;
}

} // namespace Hylux::RHI
