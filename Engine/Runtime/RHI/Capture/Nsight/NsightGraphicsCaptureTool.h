/// @file
/// @brief IGraphicsCaptureTool implementation backed by NVIDIA Nsight Graphics (NGFX).
///        Implements the frame-capture activity (StartCapture/StopCapture); GPU trace and
///        range profiler are exposed as future sub-interface attachments. Markers fan out
///        from VulkanCommandList already invoke vkCmdBeginDebugUtilsLabelEXT, which NGFX
///        hooks automatically, so PushMarker/PopMarker are no-ops here.

#pragma once

#include "RHI/Capture/IGraphicsCaptureTool.h"

#include <volk.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Hylux::RHI::Vulkan
{

class NsightGraphicsCaptureTool final : public IGraphicsCaptureTool
{
public:
    explicit NsightGraphicsCaptureTool(VkInstance instance);
    ~NsightGraphicsCaptureTool() override;

    [[nodiscard]] CaptureToolKind GetKind() const noexcept override
    {
        return CaptureToolKind::Nsight;
    }
    [[nodiscard]] bool          IsAvailable() const noexcept override { return available_; }
    [[nodiscard]] std::uint32_t GetSdkVersion() const noexcept override;
    [[nodiscard]] bool          SupportsActivity(CaptureActivity activity) const noexcept override;

    Ref<ICaptureScope> BeginScope(std::string_view name) override;
    std::uint64_t      RequestCapture(const CaptureRequest& request) override;
    bool               CancelCapture(std::uint64_t requestId) override;

    [[nodiscard]] CaptureStatus    GetCaptureStatus(std::uint64_t requestId) const noexcept override;
    [[nodiscard]] std::string_view GetCapturePath(std::uint64_t requestId) const noexcept override;

    void FrameBoundary() override;

    void PushMarker(IRHICommandList* commandList, std::string_view name,
                    CaptureMarkerColor color) override;
    void PopMarker(IRHICommandList* commandList) override;
    void InsertMarker(IRHICommandList* commandList, std::string_view name,
                      CaptureMarkerColor color) override;

    [[nodiscard]] IGpuRangeProfiler* GetRangeProfiler() noexcept override { return nullptr; }
    [[nodiscard]] IGpuTraceCapture*  GetTraceCapture()  noexcept override { return nullptr; }
    [[nodiscard]] IShaderProfiler*   GetShaderProfiler() noexcept override { return nullptr; }
    [[nodiscard]] void*              GetNativeContext() const noexcept override { return nullptr; }

private:
    VkInstance   instance_{VK_NULL_HANDLE};
    bool         available_{false};
    std::atomic<std::uint64_t> nextRequestId_{1};

    struct RequestState
    {
        CaptureStatus status{CaptureStatus::Idle};
        std::string   outputPath;
    };
    mutable std::mutex                                requestsMutex_;
    std::unordered_map<std::uint64_t, RequestState>   requests_;
};

} // namespace Hylux::RHI::Vulkan
