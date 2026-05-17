/// @file
/// @brief Abstract graphics capture / GPU profiler integration point. Wraps Nsight Graphics
///        SDK, PIX, RenderDoc, Radeon GPU Profiler — engine code uses the same call sites
///        regardless of which SDK is loaded. Tool selection and loading is driven by the
///        InstanceDesc and gated on validation being enabled. Frame capture is the baseline
///        activity; GPU trace, range/PerfWorks-style metric profiling, and shader profiling
///        are exposed through optional sub-interfaces fetched via the GetXxx accessors.

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Hylux::RHI
{

/// @brief RGBA color used to tint debug markers in capture timelines.
struct CaptureMarkerColor
{
    std::uint8_t r{0xFF};
    std::uint8_t g{0xFF};
    std::uint8_t b{0xFF};
    std::uint8_t a{0xFF};
};

/// @brief Parameters for a single IGraphicsCaptureTool::RequestCapture call. Fields not
///        relevant to the chosen activity are ignored by the backend.
struct CaptureRequest
{
    CaptureActivity activity{CaptureActivity::FrameCapture};
    std::uint32_t   numFrames{1};
    std::string     outputPath{};
    std::string     captureName{};
    bool            includeShaderSource{false};
    bool            serializeResources{true};
    bool            openInUiOnComplete{false};
};

/// @brief RAII scope object returned by IGraphicsCaptureTool::BeginScope. End is called
///        explicitly or implicitly on destruction.
class ICaptureScope : public RefCounted
{
public:
    /// @brief Ends the capture scope immediately. Safe to call multiple times; subsequent
    ///        calls are no-ops.
    virtual void End() = 0;
};

/// @brief Abstract capture SDK wrapper exposed through IRHIInstance::GetCaptureTool and
///        IRHIDevice::GetCaptureTool. Methods are no-ops when the underlying SDK is absent;
///        sub-interface accessors return nullptr when the SDK does not implement that
///        capability.
class IGraphicsCaptureTool : public RefCounted
{
public:
    /// @brief Returns the concrete SDK kind this object wraps.
    [[nodiscard]] virtual CaptureToolKind GetKind() const noexcept = 0;

    /// @brief Returns true if the SDK was successfully loaded and is active.
    [[nodiscard]] virtual bool IsAvailable() const noexcept = 0;

    /// @brief Returns the version of the loaded SDK, or 0 if unavailable.
    [[nodiscard]] virtual std::uint32_t GetSdkVersion() const noexcept = 0;

    /// @brief Returns true if the loaded SDK can service the given activity. Always returns
    ///        false when IsAvailable is false.
    [[nodiscard]] virtual bool SupportsActivity(CaptureActivity activity) const noexcept = 0;

    /// @brief Begins a named capture scope. The returned ICaptureScope ends the scope on
    ///        release if End was not called explicitly.
    virtual Ref<ICaptureScope> BeginScope(std::string_view name) = 0;

    /// @brief Submits a capture request to the loaded SDK. Returns a non-zero request id on
    ///        success, or 0 if the SDK is unavailable or the activity is unsupported. The
    ///        capture runs asynchronously; poll status via GetCaptureStatus.
    virtual std::uint64_t RequestCapture(const CaptureRequest& request) = 0;

    /// @brief Cancels an in-flight capture if the backend supports cancellation. Returns
    ///        false if the request is unknown or has already completed.
    virtual bool CancelCapture(std::uint64_t requestId) = 0;

    /// @brief Returns the lifecycle state of a previously submitted request. Unknown ids
    ///        return CaptureStatus::Idle.
    [[nodiscard]] virtual CaptureStatus GetCaptureStatus(std::uint64_t requestId) const noexcept = 0;

    /// @brief Returns the on-disk path of a completed capture, or an empty view if the
    ///        request has not completed (or did not produce a file).
    [[nodiscard]] virtual std::string_view GetCapturePath(std::uint64_t requestId) const noexcept = 0;

    /// @brief Marks a frame boundary for headless workloads with no Present call.
    ///        Optional queue + output texture pin the boundary to a specific submission
    ///        path and final image (used by capture SDKs that need both for frame
    ///        delimitation and HUD compositing); pass nullptr for either when unknown.
    virtual void FrameBoundary(IRHIQueue* queue = nullptr, IRHITexture* outputTexture = nullptr) = 0;

    /// @brief Pushes a named debug region on the command list. Routed by IRHICommandList
    ///        when a tool is loaded; not normally called directly by gameplay code.
    virtual void PushMarker(IRHICommandList* commandList, std::string_view name,
                            CaptureMarkerColor color) = 0;

    /// @brief Pops the most recent debug region on the command list.
    virtual void PopMarker(IRHICommandList* commandList) = 0;

    /// @brief Inserts a single-point debug marker on the command list.
    virtual void InsertMarker(IRHICommandList* commandList, std::string_view name,
                              CaptureMarkerColor color) = 0;

    /// @brief Returns the range / PerfWorks-style counter profiler, or nullptr if the loaded
    ///        SDK does not expose counter sampling.
    [[nodiscard]] virtual IGpuRangeProfiler* GetRangeProfiler() noexcept = 0;

    /// @brief Returns the GPU trace controller, or nullptr if the loaded SDK does not
    ///        expose timeline-based tracing.
    [[nodiscard]] virtual IGpuTraceCapture* GetTraceCapture() noexcept = 0;

    /// @brief Returns the shader profiler, or nullptr if the loaded SDK does not expose
    ///        SASS / program-counter sampling.
    [[nodiscard]] virtual IShaderProfiler* GetShaderProfiler() noexcept = 0;

    /// @brief Returns the opaque native context for the loaded SDK. Consumers cast to the
    ///        SDK-specific type (e.g. NGFX handle, IDXGraphicsAnalysis*) in their own .cpp.
    [[nodiscard]] virtual void* GetNativeContext() const noexcept = 0;
};

} // namespace Hylux::RHI
