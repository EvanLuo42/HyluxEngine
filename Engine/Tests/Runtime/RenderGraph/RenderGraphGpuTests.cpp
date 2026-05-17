/// @file
/// @brief Per-backend RenderGraph smoke tests. Build a tiny graph with a transient buffer,
///        compile, execute on a recorded command list, and confirm pass ordering + the
///        ValidationLogGuard captures nothing.

#include "../RHI/GpuCase.h"

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHICommandPool.h"
#include "RHI/IRHIDevice.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGContext.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RenderGraph.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RenderGraph");

#include <vector>

using namespace Hylux;
using namespace Hylux::Tests;

namespace
{

/// @brief Tiny compute-style pass that records the order in which the graph executes it.
class TracePass final : public RG::RGPass
{
public:
    TracePass(std::vector<int>& trace, int id, RG::RGBufferHandle& read, RG::RGBufferHandle& write,
              bool sideEffect = false)
        : trace_(&trace), id_(id), readHandle_(&read), writeHandle_(&write), sideEffect_(sideEffect)
    {}

    void Setup(RG::RGBuilder& builder) override
    {
        if (readHandle_->IsValid())
        {
            builder.ReadBuffer(*readHandle_, RG::RGBufferAccess{});
        }
        if (writeHandle_->IsValid())
        {
            *writeHandle_ = builder.WriteBuffer(*writeHandle_, RG::RGBufferAccess{
                RHI::PipelineStageMask::ComputeShader,
                RHI::AccessMask::ShaderResourceWrite});
        }
        if (sideEffect_) builder.SetSideEffect();
    }

    void Execute(RG::RGContext&, RHI::IRHICommandList&) override
    {
        trace_->push_back(id_);
    }

private:
    std::vector<int>*     trace_;
    int                   id_;
    RG::RGBufferHandle*   readHandle_;
    RG::RGBufferHandle*   writeHandle_;
    bool                  sideEffect_;
};

} // namespace

GPU_CASE("RenderGraph: side-effect pass survives dead-pass culling and Execute is invoked",
         Backends::All)
{
    RG::RenderGraph graph{gpu.device.Get()};
    std::vector<int> trace;

    RG::RGBufferHandle dummyRead;
    RG::RGBufferHandle dummyWrite;
    graph.AddPass<TracePass>("SideEffect", trace, 1, dummyRead, dummyWrite, true);
    graph.Compile();

    auto pool = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    auto cmd  = pool ? pool->AllocateCommandList() : Ref<RHI::IRHICommandList>{};
    REQUIRE(cmd);
    REQUIRE(cmd->Begin());
    graph.Execute(*cmd);
    REQUIRE(cmd->End());

    REQUIRE(trace.size() == 1u);
    CHECK(trace[0] == 1);
}

GPU_CASE("RenderGraph: producer/consumer dependency orders writer before reader",
         Backends::All)
{
    RG::RenderGraph graph{gpu.device.Get()};
    std::vector<int> trace;

    RG::RGBufferHandle producedHandle;

    // Producer creates a buffer and writes to it.
    class ProducerPass final : public RG::RGPass
    {
    public:
        ProducerPass(std::vector<int>& trace, RG::RGBufferHandle& out) : trace_(&trace), out_(&out) {}
        void Setup(RG::RGBuilder& b) override
        {
            RHI::BufferDesc d{};
            d.size  = 64;
            d.usage = RHI::BufferUsage::StorageBuffer;
            RG::RGBufferHandle h = b.CreateBuffer("transient", d);
            *out_ = b.WriteBuffer(h, RG::RGBufferAccess{
                RHI::PipelineStageMask::ComputeShader,
                RHI::AccessMask::ShaderResourceWrite});
        }
        void Execute(RG::RGContext&, RHI::IRHICommandList&) override { trace_->push_back(0); }
    private:
        std::vector<int>*   trace_;
        RG::RGBufferHandle* out_;
    };

    class ConsumerPass final : public RG::RGPass
    {
    public:
        ConsumerPass(std::vector<int>& trace, RG::RGBufferHandle handle, bool sideEffect)
            : trace_(&trace), handle_(handle), sideEffect_(sideEffect) {}
        void Setup(RG::RGBuilder& b) override
        {
            b.ReadBuffer(handle_, RG::RGBufferAccess{});
            if (sideEffect_) b.SetSideEffect();
        }
        void Execute(RG::RGContext&, RHI::IRHICommandList&) override { trace_->push_back(1); }
    private:
        std::vector<int>*  trace_;
        RG::RGBufferHandle handle_;
        bool               sideEffect_;
    };

    graph.AddPass<ProducerPass>("Producer", trace, producedHandle);
    graph.AddPass<ConsumerPass>("Consumer", trace, producedHandle, true);
    graph.Compile();

    auto pool = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    auto cmd  = pool ? pool->AllocateCommandList() : Ref<RHI::IRHICommandList>{};
    REQUIRE(cmd);
    REQUIRE(cmd->Begin());
    graph.Execute(*cmd);
    REQUIRE(cmd->End());

    REQUIRE(trace.size() == 2u);
    CHECK(trace[0] == 0);
    CHECK(trace[1] == 1);
}

GPU_CASE("RenderGraph: passes whose outputs no live pass consumes are culled",
         Backends::All)
{
    RG::RenderGraph graph{gpu.device.Get()};
    std::vector<int> trace;

    class DeadPass final : public RG::RGPass
    {
    public:
        explicit DeadPass(std::vector<int>& trace) : trace_(&trace) {}
        void Setup(RG::RGBuilder& b) override
        {
            RHI::BufferDesc d{};
            d.size  = 16;
            d.usage = RHI::BufferUsage::StorageBuffer;
            auto h  = b.CreateBuffer("dead", d);
            (void)b.WriteBuffer(h, {});
        }
        void Execute(RG::RGContext&, RHI::IRHICommandList&) override { trace_->push_back(0); }
    private:
        std::vector<int>* trace_;
    };

    graph.AddPass<DeadPass>("Dead", trace);
    graph.Compile();

    auto pool = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    auto cmd  = pool ? pool->AllocateCommandList() : Ref<RHI::IRHICommandList>{};
    REQUIRE(cmd);
    REQUIRE(cmd->Begin());
    graph.Execute(*cmd);
    REQUIRE(cmd->End());
    CHECK(trace.empty());
}

GPU_CASE("RenderGraph: ImportBuffer wires an externally owned buffer into the graph",
         Backends::All)
{
    RHI::BufferDesc desc{};
    desc.size  = 32;
    desc.usage = RHI::BufferUsage::StorageBuffer;
    auto external = gpu.device->CreateBuffer(desc);
    REQUIRE(external);

    RG::RenderGraph graph{gpu.device.Get()};
    std::vector<int> trace;

    class UserPass final : public RG::RGPass
    {
    public:
        UserPass(std::vector<int>& trace, RHI::IRHIBuffer* external)
            : trace_(&trace), external_(external) {}
        void Setup(RG::RGBuilder& b) override
        {
            auto h = b.ImportBuffer("ext", external_);
            (void)b.WriteBuffer(h, {});
            b.SetSideEffect();
        }
        void Execute(RG::RGContext& ctx, RHI::IRHICommandList&) override
        {
            (void)ctx;
            trace_->push_back(7);
        }
    private:
        std::vector<int>* trace_;
        RHI::IRHIBuffer*  external_;
    };

    graph.AddPass<UserPass>("Import", trace, external.Get());
    graph.Compile();

    auto pool = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    auto cmd  = pool ? pool->AllocateCommandList() : Ref<RHI::IRHICommandList>{};
    REQUIRE(cmd);
    REQUIRE(cmd->Begin());
    graph.Execute(*cmd);
    REQUIRE(cmd->End());
    REQUIRE(trace.size() == 1u);
    CHECK(trace[0] == 7);
}

TEST_SUITE_END();
