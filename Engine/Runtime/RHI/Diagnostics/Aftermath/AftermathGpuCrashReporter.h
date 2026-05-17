/// @file
/// @brief IGpuCrashReporter implementation backed by NVIDIA Nsight Aftermath. Constructed
///        early by VulkanAdapter::CreateDevice so it can chain VkDeviceDiagnosticsConfigNV
///        into VkDeviceCreateInfo via AftermathDeviceExtensions; BindDevice() finalizes the
///        attachment to the live VkDevice. Markers are forwarded via vkCmdSetCheckpointNV
///        from VK_NV_device_diagnostic_checkpoints (which is the Vulkan equivalent of
///        GFSDK_Aftermath_SetEventMarker for the DX path).

#pragma once

#include "RHI/Diagnostics/IGpuCrashReporter.h"
#include "RHI/Diagnostics/Aftermath/AftermathShaderDatabase.h"

#include <volk.h>

#include <GFSDK_Aftermath_Defines.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>

namespace Hylux::RHI::Vulkan
{

/// @brief Aftermath crash reporter. Owns the dump callback state, the shader database,
///        and a small pool of strings keeping marker labels alive long enough for the
///        GPU to consume the checkpoint payload.
class AftermathGpuCrashReporter final : public IGpuCrashReporter
{
public:
    AftermathGpuCrashReporter();
    ~AftermathGpuCrashReporter() override;

    /// @brief Returns true if GFSDK_Aftermath_EnableGpuCrashDumps succeeded.
    [[nodiscard]] bool IsAvailable() const noexcept override { return enabled_; }

    /// @brief Associates the reporter with the live VkDevice after vkCreateDevice. Stores
    ///        the device for marker-extension dispatch. Idempotent.
    void BindDevice(VkDevice device) noexcept;

    [[nodiscard]] GpuCrashReporterKind GetKind() const noexcept override { return GpuCrashReporterKind::NvAftermath; }
    [[nodiscard]] std::uint32_t        GetSdkVersion() const noexcept override
    {
        return static_cast<std::uint32_t>(GFSDK_Aftermath_Version_API);
    }

    void RegisterShader(const ShaderBinaryRegistration& registration) override;
    void RegisterResource(const ResourceRegistration& registration) override;
    void UnregisterResource(IRHIObject* resource) override;

    void PushMarker(IRHICommandList* commandList, std::string_view name) override;
    void PopMarker(IRHICommandList* commandList) override;
    void InsertMarker(IRHICommandList* commandList, std::string_view name) override;

    void SetCrashDumpCallback(GpuCrashDumpCallback callback, void* userData) override;
    void SetCrashDumpDirectory(std::string_view directory) override;

    [[nodiscard]] GpuCrashStatus    PollCrashStatus() override;
    [[nodiscard]] std::string_view  GetLastDumpPath() const noexcept override { return lastDumpPath_; }
    [[nodiscard]] bool              GetFaultInfo(GpuFaultInfo& outInfo) const override;
    [[nodiscard]] void*             GetNativeContext() const noexcept override { return nullptr; }

    // RefCounted is the base; IGpuCrashReporter has no debug-name interface.

private:
    static void GFSDK_AFTERMATH_CALL CrashDumpCb(const void* dumpData, std::uint32_t dumpSize,
                                                 void* userData);
    static void GFSDK_AFTERMATH_CALL ShaderDebugInfoCb(const void* debugInfo, std::uint32_t size,
                                                       void* userData);
    static void GFSDK_AFTERMATH_CALL DescriptionCb(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue,
                                                   void* userData);
    static void GFSDK_AFTERMATH_CALL ResolveMarkerCb(
        const void* pMarkerData, std::uint32_t markerDataSize,
        void* pUserData, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker);

    /// @brief Records a marker pointer into a per-reporter ring so the checkpoint payload
    ///        outlives the GPU dispatch. Returns the stable pointer to hand to
    ///        vkCmdSetCheckpointNV. Thread-safe (cmdlists may record concurrently).
    const void* PinMarker(std::string_view name);

    AftermathShaderDatabase  shaderDb_;

    bool                     enabled_{false};
    VkDevice                 device_{VK_NULL_HANDLE};

    std::atomic<GpuCrashStatus> status_{GpuCrashStatus::NotCrashed};
    mutable std::mutex       lastDumpMutex_;
    std::string              lastDumpPath_;
    std::string              dumpDirectory_;
    GpuFaultInfo             lastFault_{};

    GpuCrashDumpCallback     userCallback_{nullptr};
    void*                    userCallbackData_{nullptr};

    std::mutex               markerMutex_;
    std::deque<std::string>  markerRing_;   // stable storage for in-flight marker strings
    std::size_t              markerRingCap_{4096};
};

} // namespace Hylux::RHI::Vulkan
