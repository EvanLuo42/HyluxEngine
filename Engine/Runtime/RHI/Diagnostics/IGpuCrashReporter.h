/// @file
/// @brief Abstract GPU crash / post-mortem dump integration point. Wraps NVIDIA Nsight
///        Aftermath, D3D12 Device Removed Extended Data (DRED), and AMD GPU Crash
///        Analyzer — engine code uses the same call sites regardless of which SDK is
///        loaded. Unlike IGraphicsCaptureTool, this backend is always-on and passive:
///        it records context continuously and emits a dump when the device reports a
///        fault. Backend selection and loading is driven by DeviceDesc::crashReporter
///        and the matching Feature::CrashReporter* entries.

#pragma once

#include "Core/Memory/RefCounted.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Descriptor for a shader binary registered with the crash reporter so the
///        backend can resolve SASS / DXIL symbols when a crash dump references this
///        shader. All borrowed buffers and string views must remain valid only for the
///        duration of the RegisterShader call; backends copy what they need.
struct ShaderBinaryRegistration
{
    std::uint64_t        shaderHash{0};
    ShaderBytecodeFormat format{ShaderBytecodeFormat::Spirv};
    ShaderStage          stage{ShaderStage::None};
    const void*          bytecode{nullptr};
    std::size_t          bytecodeSize{0};
    const void*          sourceMappingData{nullptr};
    std::size_t          sourceMappingSize{0};
    std::string_view     debugName{};
};

/// @brief Descriptor for a live resource registered with the crash reporter so the
///        backend can resolve a debug name when a crash dump references this address
///        range. Backends that auto-track resources (DRED) treat the registration as
///        a debug-name hint and ignore the address fields.
struct ResourceRegistration
{
    IRHIObject*      resource{nullptr};
    std::uint64_t    gpuVirtualAddress{0};
    std::uint64_t    sizeBytes{0};
    std::string_view debugName{};
};

/// @brief Information about a detected GPU fault, filled by GetFaultInfo. Fields the
///        backend cannot classify are left at their default-constructed values.
struct GpuFaultInfo
{
    GpuFaultKind  kind{GpuFaultKind::Unknown};
    std::uint64_t faultingVirtualAddress{0};
    std::uint64_t faultingShaderHash{0};
    std::string   faultingShaderName{};
    std::string   lastSubmittedMarker{};
    std::string   lastCompletedMarker{};
    std::string   message{};
};

/// @brief Callback fired from a backend-owned worker thread once a crash dump has been
///        finalised on disk. The callback must not call back into the device or any
///        other RHI object; treat it as a notification to flush logs and exit.
using GpuCrashDumpCallback = void (*)(std::string_view dumpPath, void* userData);

/// @brief Abstract crash reporter wrapper exposed through IRHIDevice::GetCrashReporter.
///        Methods are no-ops when the underlying SDK is absent. Designed to coexist
///        with IGraphicsCaptureTool: marker calls from IRHICommandList fan out to both
///        the capture tool and the crash reporter, while shader / resource registration
///        runs once at creation time so the dump can resolve symbols on fault.
class IGpuCrashReporter : public RefCounted
{
public:
    /// @brief Returns the concrete backend kind this object wraps.
    [[nodiscard]] virtual GpuCrashReporterKind GetKind() const noexcept = 0;

    /// @brief Returns true if the SDK was successfully loaded and is active.
    [[nodiscard]] virtual bool IsAvailable() const noexcept = 0;

    /// @brief Returns the version of the loaded SDK, or 0 if unavailable.
    [[nodiscard]] virtual std::uint32_t GetSdkVersion() const noexcept = 0;

    /// @brief Registers a shader binary so the backend can resolve symbols when a crash
    ///        dump references this shader. Safe to call from any thread; backends
    ///        serialise internally. Re-registering an existing hash replaces the prior
    ///        registration.
    virtual void RegisterShader(const ShaderBinaryRegistration& registration) = 0;

    /// @brief Registers a live resource so the backend can resolve a debug name when a
    ///        crash dump references this resource. Backends that auto-track resources
    ///        treat this as a name-only hint.
    virtual void RegisterResource(const ResourceRegistration& registration) = 0;

    /// @brief Removes a previously registered resource by pointer. No-op if the
    ///        resource was not registered.
    virtual void UnregisterResource(IRHIObject* resource) = 0;

    /// @brief Pushes a named marker into the crash dump event timeline. Routed by
    ///        IRHICommandList alongside the capture-tool marker; not normally called
    ///        directly by gameplay code.
    virtual void PushMarker(IRHICommandList* commandList, std::string_view name) = 0;

    /// @brief Pops the most recent marker on the command list.
    virtual void PopMarker(IRHICommandList* commandList) = 0;

    /// @brief Inserts a single-point marker on the command list.
    virtual void InsertMarker(IRHICommandList* commandList, std::string_view name) = 0;

    /// @brief Sets the callback fired from a backend-owned worker thread once a dump
    ///        has been finalised on disk. Pass nullptr to clear. userData is passed
    ///        through unchanged.
    virtual void SetCrashDumpCallback(GpuCrashDumpCallback callback, void* userData) = 0;

    /// @brief Configures the directory where crash dumps are written. Empty string
    ///        restores the backend default. Backends that stream dumps over IPC
    ///        ignore this setting.
    virtual void SetCrashDumpDirectory(std::string_view directory) = 0;

    /// @brief Polls the backend for crash status. Call after a submit or present
    ///        reports device-lost / TDR. Returns Collecting while the backend is
    ///        still resolving; DumpReady once the dump file exists on disk.
    [[nodiscard]] virtual GpuCrashStatus PollCrashStatus() = 0;

    /// @brief Returns the path of the most recently completed crash dump, or an empty
    ///        view if no dump has been written. Stable for the lifetime of the
    ///        reporter once non-empty.
    [[nodiscard]] virtual std::string_view GetLastDumpPath() const noexcept = 0;

    /// @brief Fills outInfo with classification and last-known marker state for the
    ///        most recently detected fault. Returns false if no fault has been
    ///        observed or if the backend cannot classify faults.
    [[nodiscard]] virtual bool GetFaultInfo(GpuFaultInfo& outInfo) const = 0;

    /// @brief Returns the opaque native context for the loaded SDK. Consumers cast to
    ///        the SDK-specific type (e.g. GFSDK_Aftermath_ContextHandle,
    ///        ID3D12DeviceRemovedExtendedData*) in their own .cpp.
    [[nodiscard]] virtual void* GetNativeContext() const noexcept = 0;
};

} // namespace Hylux::RHI
