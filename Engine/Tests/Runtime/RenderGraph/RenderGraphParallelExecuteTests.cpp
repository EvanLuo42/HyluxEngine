/// @file
/// @brief End-to-end smoke test for RenderGraph::ExecuteParallel. Builds a small graph,
///        records it via a per-worker primary-CL fan-out backed by WorkerThreadPool,
///        submits the resulting ordered CL list in one queue submit, and asserts the
///        validation log stays clean.

#include "../RHI/GpuCase.h"

#include "Core/Async/WorkerThreadPool.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHICommandPool.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIQueue.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIResourceDesc.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGContext.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RGRasterBuilder.h"
#include "RenderGraph/RGRasterPass.h"
#include "RenderGraph/RenderGraph.h"

#include <doctest/doctest.h>

#include <array>
#include <vector>

TEST_SUITE_BEGIN("RenderGraph");

using namespace Hylux;
using namespace Hylux::Tests;

namespace
{

class TrivialRasterPass final : public RG::RGRasterPass
{
public:
    TrivialRasterPass(RHI::IRHITexture* target, RG::RGTextureHandle& chain, bool mergeable, bool isFirst)
        : target_(target), chain_(&chain), mergeable_(mergeable), isFirst_(isFirst)
    {}
    void Setup(RG::RGRasterBuilder& b) override
    {
        if (isFirst_)
        {
            *chain_ = b.ImportTexture("rt", target_, RHI::ImageLayout::Undefined);
        }
        const RHI::LoadOp loadOp = isFirst_ ? RHI::LoadOp::Clear : RHI::LoadOp::Load;
        *chain_ = b.SetColorAttachment(0, *chain_, loadOp, RHI::StoreOp::Store, RHI::ClearValue{});
        b.SetSideEffect();
    }
    void Execute(RG::RGContext&, RHI::IRHICommandList&) override {}

    [[nodiscard]] bool CanMergeWith(const RG::RGRasterPass&) const noexcept override { return mergeable_; }

private:
    RHI::IRHITexture*    target_;
    RG::RGTextureHandle* chain_;
    bool                 mergeable_;
    bool                 isFirst_;
};

Ref<RHI::IRHITexture> MakeColorTarget(RHI::IRHIDevice& device)
{
    RHI::TextureDesc desc{};
    desc.dimension   = RHI::TextureDimension::Tex2D;
    desc.format      = RHI::Format::RGBA8Unorm;
    desc.extent      = {64, 64, 1};
    desc.mipLevels   = 1;
    desc.arrayLayers = 1;
    desc.sampleCount = 1;
    desc.usage       = RHI::TextureUsage::ColorAttachment;
    return device.CreateTexture(desc);
}

} // namespace

GPU_CASE("RenderGraph::ExecuteParallel: multi-pass batch records via worker fan-out and submits clean",
         Backends::All)
{
    auto target = MakeColorTarget(*gpu.device);
    REQUIRE(target);

    WorkerThreadPool pool;
    const int        workerCount = pool.GetWorkerCount();

    std::vector<Ref<RHI::IRHICommandPool>> workerPools(static_cast<std::size_t>(workerCount));
    for (int i = 0; i < workerCount; ++i)
    {
        workerPools[static_cast<std::size_t>(i)] = gpu.device->CreateCommandPool(
            RHI::QueueType::Graphics, RHI::CommandPoolFlagBits::AllowIndividualReset);
        REQUIRE(workerPools[static_cast<std::size_t>(i)]);
    }
    Ref<RHI::IRHICommandPool> mainThreadPool = gpu.device->CreateCommandPool(
        RHI::QueueType::Graphics, RHI::CommandPoolFlagBits::AllowIndividualReset);
    REQUIRE(mainThreadPool);

    RG::RenderGraph     graph{gpu.device.Get()};
    RG::RGTextureHandle chain;
    graph.AddPass<TrivialRasterPass>("p0", target.Get(), chain, /*mergeable*/ false, /*isFirst*/ true);
    graph.AddPass<TrivialRasterPass>("p1", target.Get(), chain, /*mergeable*/ true,  /*isFirst*/ false);
    graph.AddPass<TrivialRasterPass>("p2", target.Get(), chain, /*mergeable*/ true,  /*isFirst*/ false);

    graph.Compile();

    RG::RenderGraph::ParallelExecuteParams params{};
    params.workerExec    = &pool.GetExecutor();
    params.acquireCmdList = [&](int workerIndex) -> Ref<RHI::IRHICommandList> {
        if (workerIndex < 0 || workerIndex >= workerCount)
        {
            return mainThreadPool->AllocateCommandList();
        }
        return workerPools[static_cast<std::size_t>(workerIndex)]->AllocateCommandList();
    };

    std::vector<Ref<RHI::IRHICommandList>> ordered;
    graph.ExecuteParallel(params, ordered);
    REQUIRE(!ordered.empty());

    std::vector<RHI::IRHICommandList*> rawCmds;
    rawCmds.reserve(ordered.size());
    for (auto& c : ordered)
    {
        rawCmds.push_back(c.Get());
    }

    Ref<RHI::IRHIQueue> queue = gpu.device->GetQueue(RHI::QueueType::Graphics, 0);
    REQUIRE(queue);

    RHI::SubmitDesc submit{};
    submit.commandLists = rawCmds;
    std::array<RHI::SubmitDesc, 1> batches{submit};
    CHECK(queue->Submit(batches));
    CHECK(queue->WaitIdle());
}

TEST_SUITE_END();
