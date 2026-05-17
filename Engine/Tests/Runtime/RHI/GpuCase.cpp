/// @file
/// @brief GPU_CASE harness implementation. TryCreateBackend brings up the requested RHI
///        stack with engine-side validation forced to Standard so contract violations
///        surface as Error/Fatal log records that ValidationLogGuard captures.

#include "GpuCase.h"

#include "RHI/CreateRHIInstance.h"

namespace Hylux::Tests
{

namespace
{

constexpr const char* kSkipNotLinked  = "backend not linked in this build";
constexpr const char* kSkipNoInstance = "CreateRHIInstance returned null";
constexpr const char* kSkipNoAdapter  = "no default adapter available";
constexpr const char* kSkipNoDevice   = "IRHIAdapter::CreateDevice returned null";

/// @brief Compile-time check for whether a backend is actually linked into this build.
///        CreateRHIInstance logs FATAL (which aborts) when asked for an unlinked backend,
///        so we must skip the call entirely instead of relying on it returning null.
[[nodiscard]] constexpr bool IsBackendLinked(Backend backend) noexcept
{
    switch (backend)
    {
        case Backend::Vulkan:
#ifdef HYLUX_RHI_VULKAN
            return true;
#else
            return false;
#endif
        case Backend::D3D12:
#ifdef HYLUX_RHI_D3D12
            return true;
#else
            return false;
#endif
        default:
            return false;
    }
}

} // namespace

GpuFixture TryCreateBackend(Backend backend)
{
    GpuFixture fixture{};
    fixture.backend = backend;

    if (!IsBackendLinked(backend))
    {
        fixture.skipReason = kSkipNotLinked;
        return fixture;
    }

    RHI::InstanceDesc instanceDesc{};
    instanceDesc.preferredDevice = ToDeviceType(backend);
    instanceDesc.applicationName = "HyluxRuntimeTests";
    instanceDesc.engineName      = "HyluxEngine";
    instanceDesc.rhiValidation   = RHI::RhiValidationLevel::Standard;
    instanceDesc.gapiValidation  = RHI::GapiValidationLevel::Standard;
    instanceDesc.captureTool     = RHI::CaptureToolKind::None;

    fixture.instance = RHI::CreateRHIInstance(instanceDesc);
    if (!fixture.instance)
    {
        fixture.skipReason = kSkipNoInstance;
        return fixture;
    }

    fixture.adapter = fixture.instance->GetDefaultAdapter();
    if (!fixture.adapter)
    {
        fixture.skipReason = kSkipNoAdapter;
        return fixture;
    }

    RHI::DeviceDesc deviceDesc{};
    deviceDesc.graphicsQueueCount = 1;
    deviceDesc.computeQueueCount  = 0;
    deviceDesc.copyQueueCount     = 0;
    deviceDesc.rhiValidation      = RHI::RhiValidationLevel::Standard;
    deviceDesc.gapiValidation     = RHI::GapiValidationLevel::Standard;
    deviceDesc.crashReporter      = RHI::GpuCrashReporterKind::None;

    fixture.device = fixture.adapter->CreateDevice(deviceDesc);
    if (!fixture.device)
    {
        fixture.skipReason = kSkipNoDevice;
        return fixture;
    }

    fixture.supported  = true;
    fixture.skipReason = "";
    return fixture;
}

} // namespace Hylux::Tests
