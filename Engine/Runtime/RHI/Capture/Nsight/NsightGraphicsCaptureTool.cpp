/// @file
/// @brief NsightGraphicsCaptureTool implementation.
///
/// NGFX 0.9 is a header-only SDK: every entry point is an inline C function that
/// dispatches through a process-global function table (g_NGFX_Globals). The table is
/// populated lazily by NGFX_GraphicsCapture_InitializeActivity_Vulkan, which loads the
/// activity's target library (ngfx-capture-interception.dll) — but ONLY when the
/// matching injection library (ngfx-capture-injection.dll) is already in the process.
/// Nsight Graphics injects that library when it launches the target, so:
///   * Standalone runs: InitializeActivity returns NGFX_Result_InvalidState, available_
///     stays false, every other method short-circuits.
///   * Under Nsight Graphics: InitializeActivity succeeds and FrameBoundary /
///     RequestCapture / Start+Stop drive the captured-frame state machine.
///
/// Because the engine renders into an offscreen IRHITexture (no Present), captures are
/// delimited by NGFX_GraphicsCapture_Delimiter_FrameBoundary; the renderer calls
/// FrameBoundary once per submitted frame from RenderThread with the engine's graphics
/// queue and the SceneView's external output image.

#include "RHI/Capture/Nsight/NsightGraphicsCaptureTool.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "RHI/IRHIQueue.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHINativeHandle.h"

#include <NGFX_GraphicsCapture_Common.h>
#include <NGFX_GraphicsCapture_Vulkan.h>
#include <NGFX_Vulkan.h>

#include <algorithm>

namespace Hylux::RHI::Vulkan
{

namespace
{

const char* ResultToString(NGFX_Result r) noexcept
{
    switch (r)
    {
        case NGFX_Result_Success:                   return "Success";
        case NGFX_Result_NotImplemented:            return "NotImplemented";
        case NGFX_Result_LibNotFound:               return "LibNotFound";
        case NGFX_Result_InvalidLib:                return "InvalidLib";
        case NGFX_Result_DifferentActivityInjected: return "DifferentActivityInjected";
        case NGFX_Result_InvalidParameter:          return "InvalidParameter";
        case NGFX_Result_InvalidState:              return "InvalidState (Nsight not attached?)";
        case NGFX_Result_UnspecifiedError:          return "UnspecifiedError";
        case NGFX_Result_Timeout:                   return "Timeout";
        default:                                    return "Unknown";
    }
}

VkQueue ToVkQueue(IRHIQueue* queue) noexcept
{
    if (queue == nullptr) return VK_NULL_HANDLE;
    const RHINativeHandle h = queue->GetNativeHandle(NativeHandleQuery::Primary);
    if (h.kind != RHINativeHandleKind::VkQueue) return VK_NULL_HANDLE;
    return reinterpret_cast<VkQueue>(static_cast<std::uintptr_t>(h.value));
}

VkImage ToVkImage(IRHITexture* texture) noexcept
{
    if (texture == nullptr) return VK_NULL_HANDLE;
    const RHINativeHandle h = texture->GetNativeHandle(NativeHandleQuery::Primary);
    if (h.kind != RHINativeHandleKind::VkImage) return VK_NULL_HANDLE;
    return reinterpret_cast<VkImage>(static_cast<std::uintptr_t>(h.value));
}

class NsightCaptureScope final : public ICaptureScope
{
public:
    NsightCaptureScope(NsightGraphicsCaptureTool* owner) : owner_(owner) {}
    ~NsightCaptureScope() override { End(); }

    void End() override
    {
        if (ended_) return;
        ended_ = true;
        if (owner_ != nullptr) owner_->EndStartedScope();
    }

private:
    NsightGraphicsCaptureTool* owner_{nullptr};
    bool                       ended_{false};
};

} // namespace

void NsightGraphicsCaptureTool::EnsureLoaderInstalled() noexcept
{
    static std::once_flag once;
    std::call_once(once, []() {
        // Development-grade loader: no signature verification of the loaded NGFX runtime.
        // Acceptable for an editor build; ship builds can replace with a signed-only loader.
        NGFX_SetLibraryLoadFn(NGFX_LoadLib_NoVerification);
    });
}

NsightGraphicsCaptureTool::NsightGraphicsCaptureTool(VkInstance instance) : instance_(instance)
{
    EnsureLoaderInstalled();

    NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params init{};
    init.version = NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params_VER;
    const NGFX_Result r = NGFX_GraphicsCapture_InitializeActivity_Vulkan(&init);

    if (r == NGFX_Result_Success)
    {
        available_ = true;
        HYLUX_LOG(::Hylux::LogRender, Info,
                  "NGFX GraphicsCapture activity initialized (Vulkan); per-frame "
                  "FrameBoundary + manual capture API active.");
    }
    else if (r == NGFX_Result_InvalidState || r == NGFX_Result_LibNotFound)
    {
        // Expected when the engine is not launched under Nsight Graphics. Silent.
        available_ = false;
    }
    else
    {
        available_ = false;
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "NGFX GraphicsCapture initialization failed: {}", ResultToString(r));
    }
}

NsightGraphicsCaptureTool::~NsightGraphicsCaptureTool() = default;

std::uint32_t NsightGraphicsCaptureTool::GetSdkVersion() const noexcept
{
    // NGFX SDK does not expose a runtime version query; return the compile-time SDK we built
    // against (0.9.0 → 0x00090000). Callers should treat this as "the SDK version the engine
    // is wired to use", not the version of the attached Nsight Graphics installation.
    return available_ ? 0x00090000u : 0u;
}

bool NsightGraphicsCaptureTool::SupportsActivity(CaptureActivity activity) const noexcept
{
    if (!available_) return false;
    return activity == CaptureActivity::FrameCapture;
}

Ref<ICaptureScope> NsightGraphicsCaptureTool::BeginScope(std::string_view name)
{
    if (!available_) return nullptr;

    NGFX_GraphicsCapture_StartCapture_Vulkan_Params params{};
    params.version = NGFX_GraphicsCapture_StartCapture_Vulkan_Params_VER;
    const NGFX_Result r = NGFX_GraphicsCapture_StartCapture_Vulkan(&params);
    if (r != NGFX_Result_Success)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "NGFX StartCapture failed (scope='{}'): {}", name, ResultToString(r));
        return nullptr;
    }

    HYLUX_LOG(::Hylux::LogRender, Info, "NGFX StartCapture ok (scope='{}')", name);
    return MakeRef<NsightCaptureScope>(this);
}

void NsightGraphicsCaptureTool::EndStartedScope() noexcept
{
    if (!available_) return;
    NGFX_GraphicsCapture_StopCapture_Vulkan_Params params{};
    params.version = NGFX_GraphicsCapture_StopCapture_Vulkan_Params_VER;
    const NGFX_Result r = NGFX_GraphicsCapture_StopCapture_Vulkan(&params);
    if (r != NGFX_Result_Success)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn, "NGFX StopCapture failed: {}", ResultToString(r));
    }
}

std::uint64_t NsightGraphicsCaptureTool::RequestCapture(const CaptureRequest& request)
{
    if (!available_ || !SupportsActivity(request.activity)) return 0;

    NGFX_GraphicsCapture_RequestCapture_Vulkan_Params params{};
    params.version           = NGFX_GraphicsCapture_RequestCapture_Vulkan_Params_VER;
    params.delimiter         = NGFX_GraphicsCapture_Delimiter_FrameBoundary;
    params.framesBeforeStart = 0;
    params.framesToCapture   = std::clamp<std::uint32_t>(request.numFrames == 0u ? 1u : request.numFrames, 1u, 60u);

    const NGFX_Result r = NGFX_GraphicsCapture_RequestCapture_Vulkan(&params);
    if (r != NGFX_Result_Success)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "NGFX RequestCapture failed (name='{}', frames={}): {}",
                  request.captureName, params.framesToCapture, ResultToString(r));
        return 0;
    }

    const std::uint64_t id = nextRequestId_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(requestsMutex_);
        auto& state            = requests_[id];
        state.status           = CaptureStatus::Pending;
        state.framesRemaining  = static_cast<std::int32_t>(params.framesBeforeStart + params.framesToCapture);
        state.captureName      = request.captureName;
        activeRequestId_       = id;
    }
    HYLUX_LOG(::Hylux::LogRender, Info,
              "NGFX RequestCapture queued (id={}, frames={}, name='{}')",
              id, params.framesToCapture, request.captureName);
    return id;
}

bool NsightGraphicsCaptureTool::CancelCapture(std::uint64_t /*requestId*/)
{
    // NGFX RequestCapture has no cancellation path. Manual Start/Stop scopes are cancelled
    // by releasing the ICaptureScope (which routes through EndStartedScope → StopCapture).
    return false;
}

CaptureStatus NsightGraphicsCaptureTool::GetCaptureStatus(std::uint64_t requestId) const noexcept
{
    std::lock_guard lock(requestsMutex_);
    const auto      it = requests_.find(requestId);
    if (it == requests_.end()) return CaptureStatus::Idle;
    return it->second.status;
}

std::string_view NsightGraphicsCaptureTool::GetCapturePath(std::uint64_t /*requestId*/) const noexcept
{
    // The capture file goes to the Nsight Graphics project's configured output directory.
    // NGFX does not expose that path back to the application.
    return {};
}

void NsightGraphicsCaptureTool::FrameBoundary(IRHIQueue* queue, IRHITexture* outputTexture)
{
    if (!available_) return;

    NGFX_FrameBoundary_Vulkan_Params params{};
    params.version = NGFX_FrameBoundary_Vulkan_Params_VER;
    params.queue   = ToVkQueue(queue);

    NGFX_ResourceDescription_Vulkan resource{};
    if (const VkImage image = ToVkImage(outputTexture); image != VK_NULL_HANDLE)
    {
        resource.version       = NGFX_ResourceDescription_Vulkan_VER;
        resource.type          = NGFX_ResourceType_Vulkan_VkImage;
        resource.image         = image;
        params.outputResources = &resource;
        params.numOutputResources = 1;
    }

    const NGFX_Result r = NGFX_FrameBoundary_Vulkan(&params);
    if (r != NGFX_Result_Success && !loggedFrameBoundaryFailure_.test_and_set(std::memory_order_relaxed))
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "NGFX FrameBoundary failed: {} (suppressing further occurrences)", ResultToString(r));
    }

    std::lock_guard lock(requestsMutex_);
    if (activeRequestId_ == 0) return;
    const auto it = requests_.find(activeRequestId_);
    if (it == requests_.end())
    {
        activeRequestId_ = 0;
        return;
    }
    auto& state = it->second;
    if (state.status == CaptureStatus::Pending) state.status = CaptureStatus::InProgress;
    if (state.framesRemaining > 0 && --state.framesRemaining <= 0)
    {
        state.status     = CaptureStatus::Completed;
        activeRequestId_ = 0;
    }
}

void NsightGraphicsCaptureTool::PushMarker(IRHICommandList* /*cl*/, std::string_view /*name*/,
                                           CaptureMarkerColor /*color*/)
{
    // VulkanCommandList already calls vkCmdBeginDebugUtilsLabelEXT, which Nsight Graphics
    // hooks transparently; nothing additional to emit here.
}

void NsightGraphicsCaptureTool::PopMarker(IRHICommandList* /*cl*/) {}

void NsightGraphicsCaptureTool::InsertMarker(IRHICommandList* /*cl*/, std::string_view /*name*/,
                                             CaptureMarkerColor /*color*/) {}

} // namespace Hylux::RHI::Vulkan
