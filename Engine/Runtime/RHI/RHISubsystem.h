/// @file
/// @brief Engine subsystem that owns the IRHIInstance + selected IRHIAdapter + IRHIDevice,
///        and applies the RHI validation wrapper when configured. Construction parameters
///        are captured into the subsystem; Initialize performs the heavy lifting.

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Reflection/TypeId.h"
#include "Engine/ISubsystem.h"
#include "RHI/IRHIAdapter.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIInstance.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIInstanceDesc.h"

#include <vector>

namespace Hylux::RHI
{

class IGraphicsCaptureTool;

/// @brief Subsystem that brings up the RHI: instance, adapter, device, validation wrapper.
///        Construct via Engine::RegisterSubsystem<RHISubsystem>(instanceDesc, deviceDesc)
///        before calling Engine::Initialize.
class RHISubsystem : public ISubsystem
{
public:
    RHISubsystem(InstanceDesc instanceDesc, DeviceDesc deviceDesc);
    ~RHISubsystem() override = default;

    RHISubsystem(const RHISubsystem&)            = delete;
    RHISubsystem& operator=(const RHISubsystem&) = delete;
    RHISubsystem(RHISubsystem&&)                 = delete;
    RHISubsystem& operator=(RHISubsystem&&)      = delete;

    /// @brief Creates the instance, picks the default adapter, creates the device, and
    ///        (if requested) wraps it in the RHI validation layer. Logs every step.
    void Initialize(Engine& engine) override;

    /// @brief Waits for the device to idle, then releases device, adapter, and instance.
    void Shutdown() override;

    /// @brief No subsystem dependencies for now.
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override;

    [[nodiscard]] IRHIInstance* GetInstance() const noexcept { return instance_.Get(); }
    [[nodiscard]] IRHIAdapter*  GetSelectedAdapter() const noexcept { return adapter_.Get(); }
    [[nodiscard]] IRHIDevice*   GetDevice() const noexcept { return device_.Get(); }
    [[nodiscard]] IGraphicsCaptureTool* GetCaptureTool() const noexcept;

private:
    InstanceDesc      instanceDesc_;
    DeviceDesc        deviceDesc_;
    Ref<IRHIInstance> instance_;
    Ref<IRHIAdapter>  adapter_;
    Ref<IRHIDevice>   device_;
};

} // namespace Hylux::RHI
