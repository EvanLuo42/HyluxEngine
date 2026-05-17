#include "Renderer/Subsystem/RenderSubsystem.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Engine/Engine.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIPipelineCache.h"
#include "RHI/IRHIQueue.h"
#include "RHI/RHISubsystem.h"
#include "Renderer/Material/MaterialInstance.h"
#include "Renderer/Material/MaterialProxyCache.h"
#include "Renderer/Mesh/MeshAsset.h"
#include "Renderer/Path/IRenderPath.h"
#include "Renderer/Path/RenderResources.h"
#include "Renderer/Proxy/ProxyRegistry.h"
#include "Renderer/Pso/PipelineLayoutCache.h"
#include "Renderer/Pso/PsoBlobStore.h"
#include "Renderer/Pso/PsoCache.h"
#include "Renderer/Thread/FrameFenceTimeline.h"
#include "Renderer/Thread/RenderThread.h"
#include "Renderer/Thread/StructuralCommand.h"
#include "Renderer/Thread/StructuralCommandQueue.h"
#include "Renderer/Thread/TransformDoubleBuffer.h"
#include "Renderer/Upload/UploadHeapManager.h"
#include "Renderer/View/SceneViewRequest.h"
#include "Shader/ShaderSubsystem.h"

#include <utility>

namespace Hylux::Renderer
{

RenderSubsystem::RenderSubsystem(RendererConfig config)
    : config_(std::move(config)), proxies_(std::make_unique<ProxyRegistry>())
{}

RenderSubsystem::~RenderSubsystem() = default;

std::vector<TypeId> RenderSubsystem::GetDependencies() const
{
    return {
        TypeIdOf<RHI::RHISubsystem>(),
        TypeIdOf<Shader::ShaderSubsystem>(),
    };
}

void RenderSubsystem::Initialize(Engine& engine)
{
    engine_ = &engine;
    rhi_ = engine.GetSubsystem<RHI::RHISubsystem>();
    shaders_ = engine.GetSubsystem<Shader::ShaderSubsystem>();
    if (rhi_ == nullptr || shaders_ == nullptr)
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: required dependencies missing (rhi={}, shaders={})",
                        rhi_ != nullptr, shaders_ != nullptr);
        return;
    }

    device_ = rhi_->GetDevice();
    if (device_ == nullptr)
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: RHISubsystem returned a null device");
        return;
    }

    graphicsQueue_ = device_->GetQueue(RHI::QueueType::Graphics, 0);
    if (!graphicsQueue_)
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: failed to acquire graphics queue");
        return;
    }

    frameFence_ = device_->CreateFence(0);
    if (!frameFence_)
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: failed to create frame fence");
        return;
    }

    backendPsoCache_ = device_->GetOrCreatePipelineCache("RendererDefault");
    if (!backendPsoCache_)
    {
        HYLUX_LOG_WARN(LogRender, "RenderSubsystem: GetOrCreatePipelineCache returned null; PSO compiles "
                                  "will run without a backend cache");
    }

    if (config_.enablePsoDiskCache && !config_.psoCacheDir.empty())
    {
        psoBlobStore_ = std::make_unique<PsoBlobStore>(config_.psoCacheDir);
    }

    layoutCache_ = std::make_unique<PipelineLayoutCache>(device_);
    psoCache_ = std::make_unique<PsoCache>(device_, shaders_, layoutCache_.get(), backendPsoCache_.Get(),
                                           psoBlobStore_.get(), std::string{"RendererDefault"});
    psoCache_->WarmFromBlobStore();

    shaderReloadCallbackId_ = shaders_->SubscribeArchiveReloaded([this]() {
        if (psoCache_ != nullptr)
        {
            psoCache_->InvalidateAll();
            HYLUX_LOG_INFO(LogRender, "PsoCache invalidated due to shader archive reload");
        }
    });

    transformBuffer_ = std::make_unique<TransformDoubleBuffer>(device_, config_.transformBufferCapacity);
    if (!transformBuffer_->IsValid())
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: TransformDoubleBuffer creation failed");
        return;
    }

    uploadHeap_ = std::make_unique<UploadHeapManager>(device_, config_.framesInFlight, config_.uploadRingBytesPerFrame);
    if (!uploadHeap_->IsValid())
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: UploadHeapManager creation failed");
        return;
    }

    resources_ = std::make_unique<RenderResources>(device_);
    materials_ = std::make_unique<MaterialProxyCache>(shaders_);
    timeline_ = std::make_unique<FrameFenceTimeline>(config_.framesInFlight);
    cmdQueue_ = std::make_unique<StructuralCommandQueue>();

    RenderThread::Deps deps{};
    deps.device = device_;
    deps.queue = graphicsQueue_.Get();
    deps.frameFence = frameFence_.Get();
    deps.cmdQueue = cmdQueue_.get();
    deps.timeline = timeline_.get();
    deps.proxies = proxies_.get();
    deps.resources = resources_.get();
    deps.transformBuffer = transformBuffer_.get();
    deps.materials = materials_.get();
    deps.psoCache = psoCache_.get();
    deps.uploadHeap = uploadHeap_.get();

    renderThread_ = std::make_unique<RenderThread>(deps);
    if (!renderThread_->Start())
    {
        HYLUX_LOG_FATAL(LogRender, "RenderSubsystem: render thread failed to start");
        return;
    }

    for (const auto& path : renderPaths_)
    {
        path->Initialize(*device_);
    }

    initialized_ = true;
    HYLUX_LOG_INFO(LogRender,
                   "RenderSubsystem initialized (framesInFlight={}, registeredPaths={}, "
                   "psoBlobDir={})",
                   config_.framesInFlight, renderPaths_.size(),
                   config_.psoCacheDir.empty() ? std::string{"<none>"} : config_.psoCacheDir.string());
}

void RenderSubsystem::Shutdown()
{
    if (!initialized_)
    {
        return;
    }

    FlushAndWaitIdle();

    renderThread_.reset();
    cmdQueue_.reset();
    timeline_.reset();
    if (materials_)
    {
        HYLUX_LOG_INFO(LogRender, "MaterialProxyCache totals: entries={}", materials_->Size());
        materials_->Clear();
    }
    materials_.reset();
    if (resources_)
    {
        HYLUX_LOG_INFO(LogRender, "RenderResources totals: slots={}", resources_->Size());
        resources_->Reset();
    }
    resources_.reset();
    uploadHeap_.reset();
    transformBuffer_.reset();

    if (shaders_ != nullptr && shaderReloadCallbackId_ != 0)
    {
        shaders_->UnsubscribeArchiveReloaded(shaderReloadCallbackId_);
        shaderReloadCallbackId_ = 0;
    }

    for (const auto& path : renderPaths_)
    {
        path->Shutdown();
    }
    renderPaths_.clear();

    if (psoCache_)
    {
        const auto& [hitCount, missCount, failureCount] = psoCache_->GetStats();
        HYLUX_LOG_INFO(LogRender, "PsoCache totals: hits={}, misses={}, failures={}, entries={}", hitCount, missCount,
                       failureCount, psoCache_->Size());
        psoCache_->FlushBlobStore();
    }
    psoCache_.reset();
    layoutCache_.reset();
    psoBlobStore_.reset();
    backendPsoCache_.Reset();
    frameFence_.Reset();
    graphicsQueue_.Reset();
    device_ = nullptr;
    shaders_ = nullptr;
    rhi_ = nullptr;
    engine_ = nullptr;
    initialized_ = false;
    HYLUX_LOG_INFO(LogRender, "RenderSubsystem shut down");
}

IRenderPath* RenderSubsystem::RegisterRenderPath(std::unique_ptr<IRenderPath> path)
{
    if (path == nullptr)
    {
        return nullptr;
    }
    IRenderPath* raw = path.get();
    if (initialized_ && device_ != nullptr)
    {
        raw->Initialize(*device_);
    }
    renderPaths_.push_back(std::move(path));
    return raw;
}

ProxyId RenderSubsystem::SpawnPrimitive(const PrimitiveSpawnDesc& desc) const
{
    if (transformBuffer_ == nullptr || cmdQueue_ == nullptr)
    {
        return ProxyId::Invalid;
    }
    const ProxyId id = transformBuffer_->AllocateSlot();
    if (!IsValid(id))
    {
        return ProxyId::Invalid;
    }
    cmdQueue_->Enqueue(SpawnPrimitiveCmd{id, desc.layerMask});

    if (desc.mesh != nullptr)
    {
        cmdQueue_->Enqueue(AssignMeshCmd{id, desc.mesh->GetHandle()});

        const auto& [minX, minY, minZ, maxX, maxY, maxZ] = desc.mesh->GetLocalBounds();
        SetBoundsCmd boundsCmd{};
        boundsCmd.id = id;
        boundsCmd.minX = minX;
        boundsCmd.minY = minY;
        boundsCmd.minZ = minZ;
        boundsCmd.maxX = maxX;
        boundsCmd.maxY = maxY;
        boundsCmd.maxZ = maxZ;
        cmdQueue_->Enqueue(boundsCmd);
    }
    return id;
}

void RenderSubsystem::UpdateTransform(const ProxyId id, const TransformMat3x4& matrix, const std::uint32_t flags) const
{
    if (transformBuffer_ == nullptr || !IsValid(id))
    {
        return;
    }
    transformBuffer_->WriteTransform(id, matrix, flags);
}

void RenderSubsystem::DespawnPrimitive(const ProxyId id) const
{
    if (cmdQueue_)
    {
        cmdQueue_->Enqueue(DespawnPrimitiveCmd{id});
    }
}

void RenderSubsystem::AssignMaterial(const ProxyId id, const MaterialInstance& instance) const
{
    if (cmdQueue_ == nullptr)
    {
        return;
    }
    AssignMaterialCmd cmd{};
    cmd.id = id;
    cmd.snapshot = instance.Snapshot();
    cmdQueue_->Enqueue(std::move(cmd));
}

void RenderSubsystem::AssignMesh(const ProxyId id, const std::uint64_t meshHandle) const
{
    if (cmdQueue_)
    {
        cmdQueue_->Enqueue(AssignMeshCmd{id, meshHandle});
    }
}

void RenderSubsystem::SetPrimitiveFlags(const ProxyId id, const std::uint32_t flags) const
{
    if (cmdQueue_)
    {
        cmdQueue_->Enqueue(SetFlagsCmd{id, flags});
    }
}

void RenderSubsystem::SetPrimitiveBounds(const ProxyId id, const PrimitiveBounds& bounds) const
{
    if (cmdQueue_ == nullptr)
    {
        return;
    }
    SetBoundsCmd cmd{};
    cmd.id = id;
    cmd.minX = bounds.minX;
    cmd.minY = bounds.minY;
    cmd.minZ = bounds.minZ;
    cmd.maxX = bounds.maxX;
    cmd.maxY = bounds.maxY;
    cmd.maxZ = bounds.maxZ;
    cmdQueue_->Enqueue(cmd);
}

void RenderSubsystem::SubmitFrame(std::span<const SceneViewRequest> views)
{
    if (!initialized_)
    {
        HYLUX_LOG_WARN(LogRender, "RenderSubsystem::SubmitFrame called before Initialize");
        return;
    }
    if (timeline_ == nullptr || cmdQueue_ == nullptr)
    {
        return;
    }

    const auto frameId = timeline_->AcquireGameFrameId();

    if (transformBuffer_ != nullptr)
    {
        transformBuffer_->PublishAndFlip(frameId);
    }

    BeginFrameCmd begin{};
    begin.frameId = frameId;
    begin.views.assign(views.begin(), views.end());
    cmdQueue_->Enqueue(std::move(begin));

    frame_.AdvanceFrameId();
}

void RenderSubsystem::WaitForFramesConsumed() const
{
    if (timeline_)
    {
        timeline_->WaitIdle();
    }
    if (renderThread_ != nullptr && frameFence_)
    {
        const std::uint64_t target = renderThread_->SnapshotStats().frameId;
        if (target > 0)
        {
            frameFence_->Wait(target);
        }
    }
}

void RenderSubsystem::FlushAndWaitIdle()
{
    if (timeline_)
    {
        timeline_->WaitIdle();
    }
    if (cmdQueue_)
    {
        cmdQueue_->Stop();
    }
    if (renderThread_)
    {
        renderThread_->Stop();
    }
    if (graphicsQueue_)
    {
        graphicsQueue_->WaitIdle();
    }
    if (device_ != nullptr)
    {
        device_->WaitIdle();
    }
}

RendererStats RenderSubsystem::GetLastFrameStats() const noexcept
{
    if (renderThread_)
    {
        return renderThread_->SnapshotStats();
    }
    return frame_.GetStats();
}

} // namespace Hylux::Renderer
