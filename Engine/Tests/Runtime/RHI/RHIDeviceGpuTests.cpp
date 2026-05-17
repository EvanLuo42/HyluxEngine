/// @file
/// @brief Per-backend RHI smoke tests driven through the GPU_CASE harness. Each test runs
///        once per backend (Vulkan / D3D12); backends that aren't linked or fail to
///        initialize are skipped. A ValidationLogGuard wrapping the test body fails the
///        subcase on any Error/Fatal log (engine RHI validation, native debug layer,
///        validation layer callbacks routed through HYLUX_LOG).

#include "GpuCase.h"

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHICommandPool.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIFence.h"
#include "RHI/IRHIQueue.h"
#include "RHI/IRHISampler.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RHI");

#include <array>
#include <cstring>

using namespace Hylux;
using namespace Hylux::Tests;

// ---- Device basics --------------------------------------------------------

GPU_CASE("device exposes adapter, instance, and feature set", Backends::All)
{
    CHECK(gpu.device->GetAdapter() == gpu.adapter.Get());
    CHECK(gpu.device->GetInstance() == gpu.instance.Get());
    CHECK(gpu.device->GetDeviceType() == ToDeviceType(gpu.backend));
    const auto& limits = gpu.device->GetLimits();
    CHECK(limits.maxTexture2DSize > 0u);
    CHECK(limits.maxPushConstantSize >= RHI::kMaxPushConstantSize);
}

GPU_CASE("device exposes the requested graphics queue", Backends::All)
{
    auto queue = gpu.device->GetQueue(RHI::QueueType::Graphics, 0);
    REQUIRE(queue);
    CHECK(queue->GetType() == RHI::QueueType::Graphics);
    CHECK(queue->GetIndex() == 0u);
    CHECK(gpu.device->GetQueueCount(RHI::QueueType::Graphics) >= 1u);
}

GPU_CASE("device WaitIdle returns true on a fresh device", Backends::All)
{
    CHECK(gpu.device->WaitIdle());
}

// ---- Buffer creation / mapping -------------------------------------------

GPU_CASE("CreateBuffer (uniform, CpuToGpu) maps for write and unmaps cleanly", Backends::All)
{
    RHI::BufferDesc desc{};
    desc.size   = 64;
    desc.usage  = RHI::BufferUsage::UniformBuffer;
    desc.memory = RHI::MemoryUsage::CpuToGpu;
    auto buf = gpu.device->CreateBuffer(desc);
    REQUIRE(buf);
    CHECK(buf->GetSize() == 64u);

    void* p = buf->Map(0, 64);
    REQUIRE(p != nullptr);
    std::memset(p, 0, 64);
    buf->Unmap();
    CHECK(buf->GetDesc().usage == RHI::BufferUsage::UniformBuffer);
}

GPU_CASE("CreateBuffer (vertex, GpuOnly) succeeds and exposes a non-zero size", Backends::All)
{
    RHI::BufferDesc desc{};
    desc.size  = 256;
    desc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::TransferDst;
    auto buf = gpu.device->CreateBuffer(desc);
    REQUIRE(buf);
    CHECK(buf->GetSize() == 256u);
}

// ---- Texture creation ----------------------------------------------------

GPU_CASE("CreateTexture (2D, RGBA8Unorm) round-trips through GetDesc", Backends::All)
{
    RHI::TextureDesc desc{};
    desc.dimension  = RHI::TextureDimension::Tex2D;
    desc.format     = RHI::Format::RGBA8Unorm;
    desc.extent     = RHI::Extent3D{4, 4, 1};
    desc.mipLevels  = 1;
    desc.arrayLayers = 1;
    desc.sampleCount = 1;
    desc.usage      = RHI::TextureUsage::SampledImage | RHI::TextureUsage::TransferDst;
    auto tex = gpu.device->CreateTexture(desc);
    REQUIRE(tex);
    const auto& got = tex->GetDesc();
    CHECK(got.format == RHI::Format::RGBA8Unorm);
    CHECK(got.extent.width == 4u);
    CHECK(got.extent.height == 4u);
}

// ---- Sampler -------------------------------------------------------------

GPU_CASE("CreateSampler (default trilinear) succeeds", Backends::All)
{
    auto sampler = gpu.device->CreateSampler(RHI::SamplerDesc{});
    REQUIRE(sampler);
}

// ---- Fence / command list round-trip -------------------------------------

GPU_CASE("CreateFence: GetCompletedValue starts at 0; host Signal advances; Wait succeeds",
         Backends::All)
{
    auto fence = gpu.device->CreateFence(0);
    REQUIRE(fence);
    CHECK(fence->GetCompletedValue() == 0u);
    CHECK(fence->Signal(1));
    CHECK(fence->Wait(1));
}

GPU_CASE("CreateCommandPool + AllocateCommandList + Begin/End round trip", Backends::All)
{
    auto pool = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    REQUIRE(pool);
    CHECK(pool->GetQueueType() == RHI::QueueType::Graphics);
    auto cmd = pool->AllocateCommandList();
    REQUIRE(cmd);
    CHECK(cmd->Begin());
    CHECK(cmd->End());
    CHECK(pool->Reset());
}

GPU_CASE("CopyBuffer between two GPU buffers + Submit + Fence wait", Backends::All)
{
    constexpr std::uint64_t kSize = 64;
    RHI::BufferDesc srcDesc{};
    srcDesc.size   = kSize;
    srcDesc.usage  = RHI::BufferUsage::TransferSrc | RHI::BufferUsage::TransferDst;
    srcDesc.memory = RHI::MemoryUsage::CpuToGpu;
    auto src = gpu.device->CreateBuffer(srcDesc);
    REQUIRE(src);
    if (void* p = src->Map(0, kSize))
    {
        std::memset(p, 0xAB, static_cast<std::size_t>(kSize));
        src->Unmap();
    }

    RHI::BufferDesc dstDesc{};
    dstDesc.size  = kSize;
    dstDesc.usage = RHI::BufferUsage::TransferDst | RHI::BufferUsage::TransferSrc;
    auto dst = gpu.device->CreateBuffer(dstDesc);
    REQUIRE(dst);

    auto queue = gpu.device->GetQueue(RHI::QueueType::Graphics, 0);
    auto pool  = gpu.device->CreateCommandPool(RHI::QueueType::Graphics, {});
    auto cmd   = pool ? pool->AllocateCommandList() : Ref<RHI::IRHICommandList>{};
    auto fence = gpu.device->CreateFence(0);
    REQUIRE(queue);
    REQUIRE(cmd);
    REQUIRE(fence);

    REQUIRE(cmd->Begin());
    cmd->CopyBuffer(dst.Get(), 0, src.Get(), 0, kSize);
    REQUIRE(cmd->End());

    RHI::FenceSignalDesc signal{};
    signal.fence     = fence.Get();
    signal.value     = 1;
    signal.stageMask = RHI::PipelineStageMask::AllCommands;
    RHI::IRHICommandList* cmds[1] = {cmd.Get()};
    RHI::SubmitDesc submit{};
    submit.commandLists = std::span<RHI::IRHICommandList* const>(cmds, 1);
    submit.signals      = std::span<const RHI::FenceSignalDesc>(&signal, 1);
    CHECK(queue->Submit(std::span<const RHI::SubmitDesc>(&submit, 1)));
    CHECK(fence->Wait(1));
}

TEST_SUITE_END();
