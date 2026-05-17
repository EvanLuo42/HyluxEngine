/// @file
/// @brief NsightGraphicsCaptureTool implementation.
///
/// NGFX is a header-only SDK whose entry points are exported by a runtime DLL that ships
/// with Nsight Graphics. The DLL is present in-process when the engine is launched under
/// Nsight Graphics or when the user has called NGFX_GraphicsCapture_Inject_Vulkan with
/// the Nsight Graphics target distribution path. In standalone runs the entry points are
/// unresolved and IsAvailable() returns false — every IGraphicsCaptureTool method then
/// short-circuits to a no-op or an Idle status, matching the contract of the interface.
///
/// The full NGFX API (versioned Params structs, FrameBoundary control, sub-interfaces for
/// SystemProfiling / GPUTrace) is more involved than v1 needs. This first cut implements
/// the frame-capture activity, leaves the others stubbed, and is structured so the missing
/// pieces can be added without rewriting the surface.

#include "RHI/Capture/Nsight/NsightGraphicsCaptureTool.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"

#include <NGFX_GraphicsCapture_Common.h>
#include <NGFX_GraphicsCapture_Vulkan.h>

namespace Hylux::RHI::Vulkan
{

namespace
{

class NsightCaptureScope final : public ICaptureScope
{
public:
    NsightCaptureScope(NsightGraphicsCaptureTool* owner, std::uint64_t requestId)
        : owner_(owner), requestId_(requestId) {}
    void End() override
    {
        if (!ended_)
        {
            ended_ = true;
            if (owner_) owner_->CancelCapture(requestId_);
        }
    }
    ~NsightCaptureScope() override { End(); }

private:
    NsightGraphicsCaptureTool* owner_;
    std::uint64_t              requestId_;
    bool                       ended_{false};
};

} // namespace

NsightGraphicsCaptureTool::NsightGraphicsCaptureTool(VkInstance instance)
    : instance_(instance)
{
    // NGFX activity init: only succeeds in-process when running under Nsight Graphics or
    // when NGFX has been injected. We don't attempt injection here — the engine should be
    // launched under Nsight Graphics for capture. We simply probe by trying to send a
    // no-op request and observing the result.
    NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params_V1 init{};
    init.version = NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params_VER;
    const NGFX_Result r = NGFX_GraphicsCapture_InitializeActivity_Vulkan(&init);
    available_ = (r == NGFX_Result_Success);
    if (available_)
    {
        HYLUX_LOG(::Hylux::LogRender, Info, "NGFX: capture activity initialized");
    }
    else
    {
        HYLUX_LOG(::Hylux::LogRender, Info,
                  "NGFX: not active (run under Nsight Graphics to enable capture; result={})",
                  static_cast<int>(r));
    }
}

NsightGraphicsCaptureTool::~NsightGraphicsCaptureTool() = default;

std::uint32_t NsightGraphicsCaptureTool::GetSdkVersion() const noexcept
{
    // NGFX headers don't expose a version macro; surface the activity Params version that
    // we link against as a proxy. Engines can compare against a known minimum.
    return static_cast<std::uint32_t>(NGFX_GraphicsCapture_RequestCapture_Vulkan_Params_VER);
}

bool NsightGraphicsCaptureTool::SupportsActivity(CaptureActivity activity) const noexcept
{
    if (!available_) return false;
    switch (activity)
    {
        case CaptureActivity::FrameCapture:
        case CaptureActivity::GenerateCppCapture:
            return true;
        case CaptureActivity::GpuTrace:
        case CaptureActivity::RangeProfiler:
        case CaptureActivity::ShaderProfiler:
            return false;  // sub-interfaces not implemented yet
    }
    return false;
}

Ref<ICaptureScope> NsightGraphicsCaptureTool::BeginScope(std::string_view name)
{
    CaptureRequest req{};
    req.captureName.assign(name);
    const std::uint64_t id = RequestCapture(req);
    return MakeRef<NsightCaptureScope>(this, id);
}

std::uint64_t NsightGraphicsCaptureTool::RequestCapture(const CaptureRequest& request)
{
    if (!available_ || !SupportsActivity(request.activity))
    {
        return 0;
    }

    NGFX_GraphicsCapture_RequestCapture_Vulkan_Params_V1 p{};
    p.version = NGFX_GraphicsCapture_RequestCapture_Vulkan_Params_VER;
    // Field details (numFrames, output path, name) are NGFX-version-specific; a richer
    // mapping goes here once the full Params struct shape is exercised.

    const NGFX_Result r = NGFX_GraphicsCapture_RequestCapture_Vulkan(&p);
    if (r != NGFX_Result_Success)
    {
        HYLUX_LOG(::Hylux::LogRender, Warn,
                  "NGFX: RequestCapture_Vulkan returned {}", static_cast<int>(r));
        return 0;
    }

    const std::uint64_t id = nextRequestId_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(requestsMutex_);
        RequestState st;
        st.status     = CaptureStatus::Pending;
        st.outputPath = request.outputPath;
        requests_.emplace(id, std::move(st));
    }
    return id;
}

bool NsightGraphicsCaptureTool::CancelCapture(std::uint64_t requestId)
{
    std::lock_guard<std::mutex> lock(requestsMutex_);
    auto it = requests_.find(requestId);
    if (it == requests_.end()) return false;
    if (it->second.status == CaptureStatus::Completed ||
        it->second.status == CaptureStatus::Failed) return false;
    it->second.status = CaptureStatus::Failed;
    return true;
}

CaptureStatus NsightGraphicsCaptureTool::GetCaptureStatus(std::uint64_t requestId) const noexcept
{
    std::lock_guard<std::mutex> lock(requestsMutex_);
    auto it = requests_.find(requestId);
    return it == requests_.end() ? CaptureStatus::Idle : it->second.status;
}

std::string_view NsightGraphicsCaptureTool::GetCapturePath(std::uint64_t requestId) const noexcept
{
    std::lock_guard<std::mutex> lock(requestsMutex_);
    auto it = requests_.find(requestId);
    if (it == requests_.end() || it->second.status != CaptureStatus::Completed) return {};
    return it->second.outputPath;
}

void NsightGraphicsCaptureTool::FrameBoundary() {}

void NsightGraphicsCaptureTool::PushMarker(IRHICommandList* /*cl*/, std::string_view /*name*/,
                                           CaptureMarkerColor /*color*/)
{
    // NGFX hooks vkCmdBeginDebugUtilsLabelEXT automatically; nothing to do here.
}
void NsightGraphicsCaptureTool::PopMarker(IRHICommandList* /*cl*/) {}
void NsightGraphicsCaptureTool::InsertMarker(IRHICommandList* /*cl*/, std::string_view /*name*/,
                                             CaptureMarkerColor /*color*/) {}

} // namespace Hylux::RHI::Vulkan
