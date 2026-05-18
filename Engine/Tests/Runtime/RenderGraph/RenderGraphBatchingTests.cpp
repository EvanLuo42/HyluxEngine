/// @file
/// @brief Tests for RenderGraph::ComputeRenderPassBatches: consecutive mergeable raster
///        passes form one batch; an interposed non-raster pass breaks the run.

#include "../RHI/GpuCase.h"

#include "Core/Memory/Ref.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIRenderPassAttachments.h"
#include "RHI/RHIResourceDesc.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGContext.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RGRasterBuilder.h"
#include "RenderGraph/RGRasterPass.h"
#include "RenderGraph/RenderGraph.h"

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("RenderGraph");

using namespace Hylux;
using namespace Hylux::Tests;

namespace
{

class ChainedRasterPass final : public RG::RGRasterPass
{
public:
    ChainedRasterPass(RHI::IRHITexture* target, RG::RGTextureHandle& chain, bool mergeable, bool isFirst)
        : target_(target), chain_(&chain), mergeable_(mergeable), isFirst_(isFirst)
    {}

    void Setup(RG::RGRasterBuilder& b) override
    {
        if (isFirst_)
        {
            *chain_ = b.ImportTexture("rt", target_, RHI::ImageLayout::ColorAttachment);
        }
        *chain_ = b.SetColorAttachment(0, *chain_, RHI::LoadOp::Load, RHI::StoreOp::Store);
        b.SetSideEffect();
    }
    void Execute(RG::RGContext&, RHI::IRHICommandList&) override {}

    [[nodiscard]] bool CanMergeWith(const RG::RGRasterPass& /*previous*/) const noexcept override
    {
        return mergeable_;
    }

private:
    RHI::IRHITexture*    target_;
    RG::RGTextureHandle* chain_;
    bool                 mergeable_;
    bool                 isFirst_;
};

class ComputeBreakerPass final : public RG::RGPass
{
public:
    explicit ComputeBreakerPass(RG::RGTextureHandle handle) : handle_(handle) {}
    void Setup(RG::RGBuilder& b) override
    {
        b.ReadTexture(handle_, RG::RGTextureAccess{});
        b.SetSideEffect();
    }
    void Execute(RG::RGContext&, RHI::IRHICommandList&) override {}

private:
    RG::RGTextureHandle handle_;
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

GPU_CASE("RenderGraph batching: three mergeable raster passes form one batch", Backends::All)
{
    auto target = MakeColorTarget(*gpu.device);
    REQUIRE(target);

    RG::RenderGraph     graph{gpu.device.Get()};
    RG::RGTextureHandle chain;
    graph.AddPass<ChainedRasterPass>("p0", target.Get(), chain, /*mergeable*/ false, /*isFirst*/ true);
    graph.AddPass<ChainedRasterPass>("p1", target.Get(), chain, /*mergeable*/ true,  /*isFirst*/ false);
    graph.AddPass<ChainedRasterPass>("p2", target.Get(), chain, /*mergeable*/ true,  /*isFirst*/ false);

    graph.Compile();

    const auto batches = graph.RenderPassBatches();
    REQUIRE(batches.size() == 1);
    CHECK(batches[0].firstExecIndex == 0);
    CHECK(batches[0].count == 3);
    CHECK(batches[0].attachments.colorAttachmentCount == 1);
    CHECK(batches[0].attachments.colorFormats[0] == RHI::Format::RGBA8Unorm);
}

GPU_CASE("RenderGraph batching: interposed compute pass breaks the run", Backends::All)
{
    auto target = MakeColorTarget(*gpu.device);
    REQUIRE(target);

    RG::RenderGraph     graph{gpu.device.Get()};
    RG::RGTextureHandle chain;
    graph.AddPass<ChainedRasterPass>("r0", target.Get(), chain, /*mergeable*/ false, /*isFirst*/ true);
    graph.AddPass<ComputeBreakerPass>("compute", chain);
    graph.AddPass<ChainedRasterPass>("r1", target.Get(), chain, /*mergeable*/ true,  /*isFirst*/ false);

    graph.Compile();

    const auto batches = graph.RenderPassBatches();
    CHECK(batches.size() == 3);
}

TEST_SUITE_END();
