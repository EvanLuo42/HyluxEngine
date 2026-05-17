/// @file
/// @brief IGraphicsCaptureTool implementation backed by NVIDIA Nsight Graphics (NGFX 0.9).
///        Wires three flows expected by the editor / renderer:
///          * Per-frame NGFX_FrameBoundary_Vulkan markers driven from RenderThread::Run.
///            The marker is anchored to the engine's graphics queue and the SceneView's
///            external output VkImage so Nsight treats those as the captured frame's focal
///            resources — Qt's own QtQuick VkInstance work is not part of the same queue.
///          * RequestCapture: one-shot N-frames capture, gated by FrameBoundary delimiter.
///            Used to wire an editor "capture frame" button.
///          * BeginScope / ICaptureScope::End: manual Start/Stop pair, also intended for
///            editor "record / stop record" buttons.
///        Push/Pop/Insert markers are no-ops: VulkanCommandList already emits
///        vkCmdBeginDebugUtilsLabelEXT, which Nsight Graphics hooks transparently.

#pragma once

#include "RHI/Capture/IGraphicsCaptureTool.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <volk.h>

namespace Hylux::RHI::Vulkan
{

class NsightGraphicsCaptureTool final : public IGraphicsCaptureTool
{
public:
    /// @brief Installs NGFX_LoadLib_NoVerification as the SDK's library-load callback.
    ///        The default callback (NGFX_LoadLib_UserOverrideRequired) prints an error and
    ///        calls abort(). When the engine is launched under Nsight Graphics the injected
    ///        capture-interception runtime can invoke NGFX entry points from inside the
    ///        vkCreateInstance hook, so this must run BEFORE vkCreateInstance. Idempotent;
    ///        safe to call multiple times and safe even when Nsight is not attached.
    static void EnsureLoaderInstalled() noexcept;

    explicit NsightGraphicsCaptureTool(VkInstance instance);
    ~NsightGraphicsCaptureTool() override;

    [[nodiscard]] CaptureToolKind GetKind() const noexcept override { return CaptureToolKind::Nsight; }
    [[nodiscard]] bool            IsAvailable() const noexcept override { return available_; }
    [[nodiscard]] std::uint32_t   GetSdkVersion() const noexcept override;
    [[nodiscard]] bool            SupportsActivity(CaptureActivity activity) const noexcept override;

    Ref<ICaptureScope> BeginScope(std::string_view name) override;
    std::uint64_t      RequestCapture(const CaptureRequest& request) override;
    bool               CancelCapture(std::uint64_t requestId) override;

    [[nodiscard]] CaptureStatus    GetCaptureStatus(std::uint64_t requestId) const noexcept override;
    [[nodiscard]] std::string_view GetCapturePath(std::uint64_t requestId) const noexcept override;

    void FrameBoundary(IRHIQueue* queue, IRHITexture* outputTexture) override;

    void PushMarker(IRHICommandList* commandList, std::string_view name, CaptureMarkerColor color) override;
    void PopMarker(IRHICommandList* commandList) override;
    void InsertMarker(IRHICommandList* commandList, std::string_view name, CaptureMarkerColor color) override;

    [[nodiscard]] IGpuRangeProfiler* GetRangeProfiler() noexcept override { return nullptr; }
    [[nodiscard]] IGpuTraceCapture*  GetTraceCapture() noexcept override { return nullptr; }
    [[nodiscard]] IShaderProfiler*   GetShaderProfiler() noexcept override { return nullptr; }
    [[nodiscard]] void*              GetNativeContext() const noexcept override { return nullptr; }

    /// @brief Stops a Start/Stop scope previously opened by BeginScope. Called from
    ///        NsightCaptureScope::End; safe to invoke when no scope is active.
    void EndStartedScope() noexcept;

private:
    VkInstance                 instance_{VK_NULL_HANDLE};
    bool                       available_{false};
    std::atomic<std::uint64_t> nextRequestId_{1};

    /// @brief Bookkeeping for a one-shot RequestCapture. NGFX itself does not expose a
    ///        completion callback, so framesRemaining is decremented in FrameBoundary and
    ///        the status flips to Completed when it reaches zero.
    struct RequestState
    {
        CaptureStatus status{CaptureStatus::Idle};
        std::int32_t  framesRemaining{0};
        std::string   captureName;
    };
    mutable std::mutex                              requestsMutex_;
    std::unordered_map<std::uint64_t, RequestState> requests_;
    std::uint64_t                                   activeRequestId_{0};

    mutable std::atomic_flag loggedFrameBoundaryFailure_{};
};

} // namespace Hylux::RHI::Vulkan
