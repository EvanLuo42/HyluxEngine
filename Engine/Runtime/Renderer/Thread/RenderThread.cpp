/// @file
/// @brief RenderThread implementation.

#include "Renderer/Thread/RenderThread.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIFence.h"
#include "RHI/IRHIQueue.h"
#include "RenderGraph/RenderGraph.h"
#include "Renderer/Material/MaterialProxy.h"
#include "Renderer/Material/MaterialProxyCache.h"
#include "Renderer/Path/IRenderPath.h"
#include "Renderer/Path/RenderContext.h"
#include "Renderer/Path/RenderResources.h"
#include "Renderer/Proxy/ProxyRegistry.h"
#include "Renderer/Thread/FrameFenceTimeline.h"
#include "Renderer/Thread/StructuralCommand.h"
#include "Renderer/Thread/StructuralCommandQueue.h"
#include "Renderer/Thread/TransformDoubleBuffer.h"
#include "Renderer/Upload/UploadHeapManager.h"
#include "Renderer/View/SceneView.h"
#include "Renderer/View/SceneViewRequest.h"

#include <array>
#include <span>
#include <utility>
#include <variant>

namespace Hylux::Renderer
{

RenderThread::RenderThread(const Deps& deps) noexcept : deps_(deps) {}

RenderThread::~RenderThread()
{
    Stop();
}

bool RenderThread::Start()
{
    if (started_.load(std::memory_order_acquire))
    {
        return true;
    }
    if (deps_.device == nullptr || deps_.queue == nullptr || deps_.cmdQueue == nullptr || deps_.timeline == nullptr)
    {
        HYLUX_LOG_ERROR(LogRender, "RenderThread::Start: missing dependency");
        return false;
    }

    started_.store(true, std::memory_order_release);
    thread_ = std::thread(&RenderThread::Run, this);
    return true;
}

void RenderThread::Stop()
{
    if (!started_.load(std::memory_order_acquire))
    {
        return;
    }
    if (deps_.cmdQueue != nullptr)
    {
        deps_.cmdQueue->Stop();
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
    started_.store(false, std::memory_order_release);
}

RendererStats RenderThread::SnapshotStats() const
{
    std::lock_guard lock(statsMutex_);
    return lastStats_;
}

void RenderThread::Run()
{
    commandPool_ =
        deps_.device->CreateCommandPool(RHI::QueueType::Graphics, RHI::CommandPoolFlagBits::AllowIndividualReset);
    if (!commandPool_)
    {
        HYLUX_LOG_FATAL(LogRender, "RenderThread: failed to create command pool; thread exiting");
        return;
    }

    const std::uint32_t framesInFlight = deps_.timeline != nullptr ? deps_.timeline->GetFramesInFlight() : 1u;
    cmdListRing_.assign(framesInFlight, Ref<RHI::IRHICommandList>{});
    slotFenceValues_.assign(framesInFlight, 0ull);

    HYLUX_LOG_INFO(LogRender, "RenderThread started");

    while (true)
    {
        std::vector<StructuralCommand> commands;
        if (hasCachedBegin_)
        {
            commands = deps_.cmdQueue->TryDrainAll();
            if (commands.empty() && deps_.cmdQueue->IsStopped())
            {
                break;
            }
        }
        else
        {
            commands = deps_.cmdQueue->DrainToFrameBoundary();
            if (commands.empty())
            {
                break;
            }
        }

        std::uint32_t beginCountConsumed = 0;
        for (auto& cmd : commands)
        {
            if (std::holds_alternative<BeginFrameCmd>(cmd))
            {
                cachedBegin_     = std::move(std::get<BeginFrameCmd>(cmd));
                hasCachedBegin_  = true;
                ++beginCountConsumed;
            }
            else
            {
                DispatchMutation(cmd);
            }
        }
        for (std::uint32_t i = 0; i < beginCountConsumed; ++i)
        {
            deps_.timeline->ReleaseRenderFrame();
        }

        if (!hasCachedBegin_)
        {
            continue;
        }

        ++renderFrameId_;
        const std::uint32_t ringSize = static_cast<std::uint32_t>(cmdListRing_.size());
        const std::uint32_t slot     = ringSize == 0 ? 0u : static_cast<std::uint32_t>(renderFrameId_ % ringSize);

        if (deps_.frameFence != nullptr && slotFenceValues_[slot] != 0ull)
        {
            deps_.frameFence->Wait(slotFenceValues_[slot]);
        }
        cmdListRing_[slot].Reset();

        auto cmdList = commandPool_->AllocateCommandList();
        bool ok      = static_cast<bool>(cmdList);
        if (ok && !cmdList->Begin())
        {
            HYLUX_LOG_ERROR(LogRender, "RenderThread: command list Begin failed (render {})", renderFrameId_);
            ok = false;
        }
        if (ok)
        {
            RenderFrame(cachedBegin_, renderFrameId_, *cmdList);
            cmdList->End();

            ++fenceValue_;
            RHI::FenceSignalDesc signal{};
            signal.fence     = deps_.frameFence;
            signal.value     = fenceValue_;
            signal.stageMask = RHI::PipelineStageMask::AllCommands;

            std::array      cmdLists{cmdList.Get()};
            RHI::SubmitDesc submit{};
            submit.commandLists = std::span<RHI::IRHICommandList* const>{cmdLists};
            submit.signals      = std::span<const RHI::FenceSignalDesc>{&signal, 1};

            if (std::array submits{submit}; !deps_.queue->Submit(std::span<const RHI::SubmitDesc>{submits}))
            {
                HYLUX_LOG_ERROR(LogRender, "RenderThread: queue submit failed (render {})", renderFrameId_);
            }
            else
            {
                cmdListRing_[slot]     = std::move(cmdList);
                slotFenceValues_[slot] = fenceValue_;

                if (auto* capture = deps_.device->GetCaptureTool())
                {
                    RHI::IRHITexture* outputTexture = cachedBegin_.views.empty()
                        ? nullptr
                        : cachedBegin_.views.front().externalTarget;
                    capture->FrameBoundary(deps_.queue, outputTexture);
                }
            }
        }

        {
            std::lock_guard lock(statsMutex_);
            lastStats_.frameId       = renderFrameId_;
            lastStats_.viewCount     = static_cast<std::uint32_t>(cachedBegin_.views.size());
            lastStats_.passCount     = 0;
            lastStats_.drawCallCount = 0;
        }
    }

    if (deps_.frameFence != nullptr && fenceValue_ != 0ull)
    {
        deps_.frameFence->Wait(fenceValue_);
    }
    cmdListRing_.clear();
    slotFenceValues_.clear();
    commandPool_.Reset();
    HYLUX_LOG_INFO(LogRender, "RenderThread exited");
}

void RenderThread::DispatchMutation(const StructuralCommand& command)
{
    if (deps_.proxies == nullptr)
    {
        return;
    }
    std::visit(
        [this]<typename T0>(const T0& payload) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, SpawnPrimitiveCmd>)
            {
                deps_.proxies->Spawn(payload.id, static_cast<std::uint32_t>(payload.id), payload.layerMask);
            }
            else if constexpr (std::is_same_v<T, DespawnPrimitiveCmd>)
            {
                deps_.proxies->Despawn(payload.id);
                if (deps_.transformBuffer != nullptr)
                {
                    deps_.transformBuffer->ReleaseSlot(payload.id);
                }
            }
            else if constexpr (std::is_same_v<T, AssignMaterialCmd>)
            {
                MaterialProxy* proxy = nullptr;
                if (deps_.materials != nullptr)
                {
                    proxy = deps_.materials->GetOrCreate(payload.snapshot);
                }
                deps_.proxies->AssignMaterial(payload.id, proxy);
            }
            else if constexpr (std::is_same_v<T, AssignMeshCmd>)
            {
                deps_.proxies->AssignMesh(payload.id, payload.meshHandle);
            }
            else if constexpr (std::is_same_v<T, SetFlagsCmd>)
            {
                deps_.proxies->SetFlags(payload.id, payload.flags);
            }
            else if constexpr (std::is_same_v<T, SetBoundsCmd>)
            {
                PrimitiveBounds b{};
                b.minX = payload.minX;
                b.minY = payload.minY;
                b.minZ = payload.minZ;
                b.maxX = payload.maxX;
                b.maxY = payload.maxY;
                b.maxZ = payload.maxZ;
                deps_.proxies->SetBounds(payload.id, b);
            }
        },
        command);
}

void RenderThread::RenderFrame(const BeginFrameCmd& begin, std::uint64_t renderFrameId,
                               RHI::IRHICommandList& cmd) const
{
    const std::uint32_t transformBindless = deps_.transformBuffer != nullptr
                                                ? deps_.transformBuffer->AcquireReadHalfBindlessIndex(begin.frameId)
                                                : TransformDoubleBuffer::kInvalidHalfIndex;

    if (deps_.uploadHeap != nullptr && deps_.timeline != nullptr)
    {
        const std::uint32_t framesInFlight = deps_.timeline->GetFramesInFlight();
        const std::uint32_t slot =
            framesInFlight == 0 ? 0u : static_cast<std::uint32_t>(renderFrameId % framesInFlight);
        deps_.uploadHeap->BeginFrame(slot);
    }

    for (const auto& request : begin.views)
    {
        if (request.path == nullptr)
        {
            continue;
        }
        SceneView       view(request);
        RG::RenderGraph graph(deps_.device);
        RenderContext   ctx(graph, view, *deps_.proxies, *deps_.resources, transformBindless, renderFrameId,
                            deps_.psoCache, deps_.transformBuffer, deps_.uploadHeap);
        request.path->BuildGraph(ctx);
        graph.Compile();
        graph.Execute(cmd);
    }

    if (deps_.resources != nullptr && deps_.timeline != nullptr)
    {
        const std::uint32_t framesInFlight = deps_.timeline->GetFramesInFlight();
        deps_.resources->EndFrame(renderFrameId, framesInFlight + 2u);
    }
}

} // namespace Hylux::Renderer
