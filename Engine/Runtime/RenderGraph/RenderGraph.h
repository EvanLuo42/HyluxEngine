/// @file
/// @brief RenderGraph orchestrator: owns the per-frame pass list, resource nodes, and the
///        Compile/Execute pipeline. Per-frame transient — construct on the stack each frame
///        from the host renderer.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/Internal/RGNode.h"
#include "RenderGraph/Internal/RGResourceRegistry.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGComputePass.h"
#include "RenderGraph/RGCopyPass.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RGRasterBuilder.h"
#include "RenderGraph/RGRasterPass.h"
#include "RenderGraph/RGRayTracingPass.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux
{
class IExecutor;
}

namespace Hylux
{
class FrameAllocator;
}

namespace Hylux::RG
{

namespace Internal { class RGTransientResourcePool; }

/// @brief Per-frame DAG of RenderGraph passes. Lifetime is one frame:
///        construct -> AddPass(s) -> Compile -> Execute. Not an ISubsystem; the future
///        RendererSubsystem owns / drives one (or pools them) per frame.
class RenderGraph
{
public:
    /// @brief Constructs a graph bound to a device, optionally backed by a frame arena
    ///        and a transient resource pool.
    ///        - `frameArena` non-null: per-compile scratch vectors allocate from it.
    ///        - `transientPool` non-null: non-imported texture/buffer nodes reuse cached
    ///          GPU resources from the pool keyed by desc. `frameId` is required when a
    ///          pool is supplied so the pool can gate reuse against frames-in-flight.
    ///        - Either pointer may be null, in which case the graph falls back to the
    ///          default heap allocator / direct CreateTexture/Buffer calls.
    explicit RenderGraph(RHI::IRHIDevice*               device,
                         FrameAllocator*                frameArena    = nullptr,
                         Internal::RGTransientResourcePool* transientPool = nullptr,
                         std::uint64_t                  frameId       = 0);
    ~RenderGraph();

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = delete;
    RenderGraph& operator=(RenderGraph&&)      = delete;

    /// @brief Constructs a TPass in-place, registers it, and immediately runs Setup with the
    ///        appropriate builder type (RGRasterBuilder if TPass derives from RGRasterPass,
    ///        otherwise RGBuilder). Returns a borrowed pointer; ownership stays with the graph.
    template<typename TPass, typename... Args>
    TPass* AddPass(std::string_view name, Args&&... args);

    /// @brief Performs dead-pass culling, topological sort, transient resource creation, and
    ///        per-pass barrier planning. Must be called before Execute.
    void Compile();

    /// @brief Records barriers, attachments, debug markers, and pass Execute calls onto the
    ///        supplied command list in compiled order.
    void Execute(RHI::IRHICommandList& cmd);

    /// @brief Parameters for the parallel execution path. See ExecuteParallel.
    struct ParallelExecuteParams
    {
        /// @brief Executor that multi-pass raster batches fan out onto. When null, every
        ///        batch records serially on the calling thread.
        IExecutor* workerExec{nullptr};

        /// @brief Allocates a fresh primary command list to record into. Called by
        ///        ExecuteParallel each time it needs another CL. workerIndex is the calling
        ///        worker thread's index (from TaskContext::WorkerIndex()) when invoked from a
        ///        fan-out task; -1 when invoked from the render thread for a single-pass
        ///        batch or the final-layout-transition tail. The caller is responsible for
        ///        choosing a pool that the calling thread owns exclusively (RHI pools are
        ///        not thread-safe).
        std::function<Ref<RHI::IRHICommandList>(int workerIndex)> acquireCmdList;
    };

    /// @brief Parallel-friendly Execute. Produces an ordered list of self-contained primary
    ///        command lists (each already Begin'd, recorded, and End'd). The caller submits
    ///        them in order in a single queue submit. Multi-pass raster batches fan out via
    ///        a TaskGraph onto params.workerExec, with each worker task allocating its own
    ///        CL through params.acquireCmdList(workerIndex). Single-pass batches and the
    ///        final-layout-transition tail are recorded on the calling thread.
    void ExecuteParallel(const ParallelExecuteParams&             params,
                         std::vector<Ref<RHI::IRHICommandList>>& outOrderedCmdLists);

    [[nodiscard]] RHI::IRHIDevice* GetDevice() const noexcept { return device_; }

    /// @brief Returns the pmr resource backing per-pass / per-compile scratch vectors.
    ///        Either the bound FrameAllocator or std::pmr::get_default_resource().
    ///        Defined out-of-line so the header does not need the FrameAllocator definition.
    [[nodiscard]] std::pmr::memory_resource* GetPmrResource() const noexcept;

    /// @brief Compile-time grouping of raster passes into shared-renderpass batches.
    ///        Populated by Compile; empty before. A trivial graph with N standalone passes
    ///        yields N single-element batches (one per pass). Used by Execute (both serial
    ///        and parallel paths) and by external tooling that wants to inspect grouping.
    [[nodiscard]] std::span<const Internal::RGRenderPassBatch> RenderPassBatches() const noexcept;

private:
    friend class RGBuilder;
    friend class RGRasterBuilder;

    [[nodiscard]] std::uint32_t CreateTextureNode(std::string_view name,
                                                  const RHI::TextureDesc& desc);
    [[nodiscard]] std::uint32_t ImportTextureNode(std::string_view name,
                                                  RHI::IRHITexture*       texture,
                                                  RHI::ImageLayout        initialLayout);
    void                        SetTextureFinalLayout(std::uint32_t textureIndex,
                                                      RHI::ImageLayout finalLayout);

    [[nodiscard]] std::uint32_t CreateBufferNode(std::string_view name,
                                                 const RHI::BufferDesc& desc);
    [[nodiscard]] std::uint32_t ImportBufferNode(std::string_view name,
                                                 RHI::IRHIBuffer* buffer);

    void RecordTextureRead(std::uint32_t passIndex, RGTextureHandle handle, RGTextureAccess access);
    [[nodiscard]] RGTextureHandle RecordTextureWrite(std::uint32_t passIndex,
                                                     RGTextureHandle handle,
                                                     RGTextureAccess access);

    void RecordBufferRead(std::uint32_t passIndex, RGBufferHandle handle, RGBufferAccess access);
    [[nodiscard]] RGBufferHandle RecordBufferWrite(std::uint32_t passIndex,
                                                   RGBufferHandle handle,
                                                   RGBufferAccess access);

    void MarkPassSideEffect(std::uint32_t passIndex);

    void RecordColorAttachment(std::uint32_t passIndex,
                               std::uint32_t slot,
                               RGTextureHandle handle,
                               RHI::LoadOp loadOp,
                               RHI::StoreOp storeOp,
                               RHI::ClearValue clearValue,
                               RGTextureHandle resolveHandle);
    void RecordDepthAttachment(std::uint32_t passIndex,
                               RGTextureHandle handle,
                               RHI::LoadOp loadOp,
                               RHI::StoreOp storeOp,
                               RHI::ClearValue clearValue);
    void RecordRenderArea(std::uint32_t passIndex, RHI::Rect2D area);

    void CullDeadPasses();
    void TopologicallySortPasses();
    void RealizeResources();
    void PlanBarriers();
    void ComputeMergeHints();
    void ComputeRenderPassBatches();

    void AccumulateTextureUsage(Internal::RGTextureNode& node, const RGTextureAccess& access);
    void AccumulateBufferUsage(Internal::RGBufferNode& node, const RGBufferAccess& access);

    void RecordPass(RHI::IRHICommandList& cmd, std::uint32_t compiledIndex);
    void RecordFinalLayoutTransitions(RHI::IRHICommandList& cmd);

    RHI::IRHIDevice*                              device_{nullptr};
    FrameAllocator*                               arena_{nullptr};
    Internal::RGTransientResourcePool*            transientPool_{nullptr};
    std::uint64_t                                 frameId_{0};
    std::vector<std::unique_ptr<RGPass>>          passOwners_;
    std::vector<Internal::RGPassNode>             passes_;
    std::vector<Internal::RGTextureNode>          textures_;
    std::vector<Internal::RGBufferNode>           buffers_;
    std::pmr::vector<std::uint32_t>               executionOrder_;
    std::vector<Internal::RGRenderPassBatch>      renderPassBatches_;
    std::vector<std::vector<std::uint32_t>>       textureVersionProducers_;
    std::vector<std::vector<std::uint32_t>>       bufferVersionProducers_;
    std::pmr::vector<Internal::RGTextureState>    currentTextureState_;
    std::pmr::vector<Internal::RGBufferState>     currentBufferState_;
    std::unique_ptr<Internal::RGResourceRegistry> registry_;
    bool                                          compiled_{false};
};

template<typename TPass, typename... Args>
TPass* RenderGraph::AddPass(std::string_view name, Args&&... args)
{
    static_assert(std::is_base_of_v<RGPass, TPass>, "TPass must derive from RGPass");

    auto owned = std::make_unique<TPass>(std::forward<Args>(args)...);
    TPass* raw = owned.get();
    raw->name_ = std::string(name);

    const std::uint32_t   passIndex = static_cast<std::uint32_t>(passes_.size());
    Internal::RGPassNode& node      = passes_.emplace_back(GetPmrResource());
    node.name     = std::string(name);
    node.pass     = raw;
    node.isRaster = std::is_base_of_v<RGRasterPass, TPass>;

    if constexpr (std::is_base_of_v<RGRasterPass, TPass>)         { node.kind = RGPassKind::Raster; }
    else if constexpr (std::is_base_of_v<RGRayTracingPass, TPass>) { node.kind = RGPassKind::RayTracing; }
    else if constexpr (std::is_base_of_v<RGComputePass, TPass>)    { node.kind = RGPassKind::Compute; }
    else if constexpr (std::is_base_of_v<RGCopyPass, TPass>)       { node.kind = RGPassKind::Copy; }
    else                                                            { node.kind = RGPassKind::Generic; }

    node.queueAffinity = raw->GetQueueAffinity();

    passOwners_.push_back(std::move(owned));

    if constexpr (std::is_base_of_v<RGRasterPass, TPass>)
    {
        RGRasterBuilder builder(this, passIndex);
        static_cast<RGRasterPass*>(raw)->Setup(static_cast<RGBuilder&>(builder));
    }
    else
    {
        RGBuilder builder(this, passIndex);
        raw->Setup(builder);
    }
    return raw;
}

} // namespace Hylux::RG
