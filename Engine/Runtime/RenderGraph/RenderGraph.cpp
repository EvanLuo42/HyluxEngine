/// @file
/// @brief RenderGraph implementation. Owns the per-frame pass list and resource node arrays,
///        runs cull/topo/realize/barrier-plan during Compile, and records barriers, dynamic
///        rendering, and per-pass Execute calls onto a command list during Execute.

#include "RenderGraph/RenderGraph.h"

#include "Core/Async/TaskGraph.h"
#include "Core/Memory/FrameAllocator.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIPipelineDesc.h"
#include "RenderGraph/Internal/RGTransientResourcePool.h"
#include "RenderGraph/RGContext.h"

#include <algorithm>
#include <cassert>
#include <memory_resource>
#include <optional>
#include <span>
#include <vector>

namespace Hylux::RG
{

namespace
{

constexpr std::uint32_t kInvalidPassIndex = 0xFFFFFFFFu;

[[nodiscard]] RHI::TextureUsage InferTextureUsage(const RGTextureAccess& access) noexcept
{
    using RHI::ImageLayout;
    using RHI::TextureUsage;
    using RHI::AccessMask;

    TextureUsage extra = TextureUsage::None;
    if ((access.access & AccessMask::InputAttachmentRead) != AccessMask::None)
    {
        extra |= TextureUsage::InputAttachment;
    }

    switch (access.layout)
    {
        case ImageLayout::ColorAttachment:
            return TextureUsage::ColorAttachment | extra;
        case ImageLayout::DepthStencilAttachment:
        case ImageLayout::DepthStencilReadOnly:
            return TextureUsage::DepthStencilAttachment | extra;
        case ImageLayout::ShaderReadOnly:
            return TextureUsage::SampledImage | extra;
        case ImageLayout::TransferSrc:
            return TextureUsage::TransferSrc | extra;
        case ImageLayout::TransferDst:
            return TextureUsage::TransferDst | extra;
        case ImageLayout::General:
            return (((access.access & AccessMask::ShaderResourceWrite) != AccessMask::None
                     || (access.access & AccessMask::ShaderResourceRead) != AccessMask::None)
                ? TextureUsage::StorageImage
                : TextureUsage::SampledImage) | extra;
        default:
            return extra;
    }
}

[[nodiscard]] RHI::BufferUsage InferBufferUsage(const RGBufferAccess& access) noexcept
{
    using RHI::AccessMask;
    using RHI::BufferUsage;

    RHI::BufferUsage usage = BufferUsage::None;
    const AccessMask a = access.access;
    if ((a & AccessMask::VertexBufferRead)    != AccessMask::None) usage |= BufferUsage::VertexBuffer;
    if ((a & AccessMask::IndexBufferRead)     != AccessMask::None) usage |= BufferUsage::IndexBuffer;
    if ((a & AccessMask::IndirectArgRead)     != AccessMask::None) usage |= BufferUsage::IndirectBuffer;
    if ((a & AccessMask::UniformRead)         != AccessMask::None) usage |= BufferUsage::UniformBuffer;
    if ((a & AccessMask::ShaderResourceRead)  != AccessMask::None) usage |= BufferUsage::StorageBuffer;
    if ((a & AccessMask::ShaderResourceWrite) != AccessMask::None) usage |= BufferUsage::StorageBuffer;
    if ((a & AccessMask::TransferRead)        != AccessMask::None) usage |= BufferUsage::TransferSrc;
    if ((a & AccessMask::TransferWrite)       != AccessMask::None) usage |= BufferUsage::TransferDst;
    return usage;
}

[[nodiscard]] bool IsWriteAccess(RHI::AccessMask access) noexcept
{
    using RHI::AccessMask;
    constexpr AccessMask kWriteMask = AccessMask::ShaderResourceWrite
                                    | AccessMask::ColorAttachmentWrite
                                    | AccessMask::DepthStencilAttachmentWrite
                                    | AccessMask::TransferWrite
                                    | AccessMask::HostWrite
                                    | AccessMask::AccelerationStructureWrite
                                    | AccessMask::MemoryWrite;
    return (access & kWriteMask) != AccessMask::None;
}

[[nodiscard]] RHI::SubresourceRange FullRange(const RHI::TextureDesc& desc) noexcept
{
    return RHI::SubresourceRange{
        0,
        desc.mipLevels,
        0,
        desc.arrayLayers,
    };
}

[[nodiscard]] RHI::TextureViewDesc MakeAttachmentViewDesc(const RHI::TextureDesc& tex,
                                                          bool isDepth) noexcept
{
    RHI::TextureViewDesc desc{};
    desc.dimension              = tex.dimension;
    desc.format                 = tex.format;
    desc.range                  = FullRange(tex);
    desc.isShaderResourceView   = false;
    desc.isUnorderedAccessView  = false;
    desc.isRenderTargetView     = !isDepth;
    desc.isDepthStencilView     = isDepth;
    return desc;
}

[[nodiscard]] std::pmr::memory_resource* ResolveResource(FrameAllocator* arena) noexcept
{
    return arena != nullptr ? arena->GetMemoryResource() : std::pmr::get_default_resource();
}

} // namespace

RenderGraph::RenderGraph(RHI::IRHIDevice* device, FrameAllocator* frameArena,
                         Internal::RGTransientResourcePool* transientPool, std::uint64_t frameId)
    : device_(device),
      arena_(frameArena),
      transientPool_(transientPool),
      frameId_(frameId),
      executionOrder_(ResolveResource(frameArena)),
      currentTextureState_(ResolveResource(frameArena)),
      currentBufferState_(ResolveResource(frameArena))
{
    assert(device_ != nullptr && "RenderGraph requires a non-null device");
    registry_ = std::make_unique<Internal::RGResourceRegistry>(device_, &textures_, &buffers_);
}

RenderGraph::~RenderGraph()
{
    if (registry_)
    {
        registry_->Reset();
    }
}

std::pmr::memory_resource* RenderGraph::GetPmrResource() const noexcept
{
    return ResolveResource(arena_);
}

std::uint32_t RenderGraph::CreateTextureNode(std::string_view name, const RHI::TextureDesc& desc)
{
    assert(!compiled_ && "Cannot mutate graph after Compile");
    Internal::RGTextureNode node;
    node.name           = std::string(name);
    node.desc           = desc;
    node.isImported    = false;
    node.initialLayout  = RHI::ImageLayout::Undefined;
    node.currentVersion = 0;
    node.inferredUsage  = RHI::TextureUsage::None;
    textures_.push_back(std::move(node));
    textureVersionProducers_.push_back(std::vector<std::uint32_t>{kInvalidPassIndex});
    return static_cast<std::uint32_t>(textures_.size() - 1);
}

std::uint32_t RenderGraph::ImportTextureNode(std::string_view  name,
                                             RHI::IRHITexture* texture,
                                             RHI::ImageLayout  initialLayout)
{
    assert(!compiled_ && "Cannot mutate graph after Compile");
    Internal::RGTextureNode node;
    node.name             = std::string(name);
    node.desc             = texture->GetDesc();
    node.isImported       = true;
    node.importedTexture  = Ref<RHI::IRHITexture>(texture);
    node.initialLayout    = initialLayout;
    node.currentVersion   = 0;
    node.inferredUsage    = RHI::TextureUsage::None;
    textures_.push_back(std::move(node));
    textureVersionProducers_.push_back(std::vector<std::uint32_t>{kInvalidPassIndex});
    return static_cast<std::uint32_t>(textures_.size() - 1);
}

void RenderGraph::SetTextureFinalLayout(std::uint32_t textureIndex, RHI::ImageLayout finalLayout)
{
    assert(!compiled_);
    assert(textureIndex < textures_.size());
    textures_[textureIndex].finalLayout = finalLayout;
}

std::uint32_t RenderGraph::CreateBufferNode(std::string_view name, const RHI::BufferDesc& desc)
{
    assert(!compiled_);
    Internal::RGBufferNode node;
    node.name           = std::string(name);
    node.desc           = desc;
    node.isImported    = false;
    node.currentVersion = 0;
    node.inferredUsage  = RHI::BufferUsage::None;
    buffers_.push_back(std::move(node));
    bufferVersionProducers_.push_back(std::vector<std::uint32_t>{kInvalidPassIndex});
    return static_cast<std::uint32_t>(buffers_.size() - 1);
}

std::uint32_t RenderGraph::ImportBufferNode(std::string_view name, RHI::IRHIBuffer* buffer)
{
    assert(!compiled_);
    Internal::RGBufferNode node;
    node.name            = std::string(name);
    node.desc            = buffer->GetDesc();
    node.isImported      = true;
    node.importedBuffer  = Ref<RHI::IRHIBuffer>(buffer);
    node.currentVersion  = 0;
    node.inferredUsage   = RHI::BufferUsage::None;
    buffers_.push_back(std::move(node));
    bufferVersionProducers_.push_back(std::vector<std::uint32_t>{kInvalidPassIndex});
    return static_cast<std::uint32_t>(buffers_.size() - 1);
}

void RenderGraph::RecordTextureRead(std::uint32_t passIndex, RGTextureHandle handle,
                                    RGTextureAccess access)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    assert(handle.Index() < textures_.size());
    AccumulateTextureUsage(textures_[handle.Index()], access);

    Internal::RGTextureAccessRecord record;
    record.textureIndex     = handle.Index();
    record.versionConsumed  = handle.Version();
    record.versionProduced  = 0;
    record.isWrite          = false;
    record.access           = access;
    passes_[passIndex].textureAccesses.push_back(record);
}

RGTextureHandle RenderGraph::RecordTextureWrite(std::uint32_t passIndex, RGTextureHandle handle,
                                                RGTextureAccess access)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    assert(handle.Index() < textures_.size());

    Internal::RGTextureNode& node = textures_[handle.Index()];
    assert(handle.Version() == node.currentVersion
           && "WriteTexture must consume the latest version of the handle");
    AccumulateTextureUsage(node, access);

    const std::uint32_t newVersion = ++node.currentVersion;

    Internal::RGTextureAccessRecord record;
    record.textureIndex     = handle.Index();
    record.versionConsumed  = handle.Version();
    record.versionProduced  = newVersion;
    record.isWrite          = true;
    record.access           = access;
    passes_[passIndex].textureAccesses.push_back(record);

    auto& producers = textureVersionProducers_[handle.Index()];
    assert(producers.size() == newVersion && "Producer table out of sync with node version");
    producers.push_back(passIndex);

    return RGTextureHandle(handle.Index(), newVersion);
}

void RenderGraph::RecordBufferRead(std::uint32_t passIndex, RGBufferHandle handle,
                                   RGBufferAccess access)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    assert(handle.Index() < buffers_.size());
    AccumulateBufferUsage(buffers_[handle.Index()], access);

    Internal::RGBufferAccessRecord record;
    record.bufferIndex      = handle.Index();
    record.versionConsumed  = handle.Version();
    record.versionProduced  = 0;
    record.isWrite          = false;
    record.access           = access;
    passes_[passIndex].bufferAccesses.push_back(record);
}

RGBufferHandle RenderGraph::RecordBufferWrite(std::uint32_t passIndex, RGBufferHandle handle,
                                              RGBufferAccess access)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    assert(handle.Index() < buffers_.size());

    Internal::RGBufferNode& node = buffers_[handle.Index()];
    assert(handle.Version() == node.currentVersion
           && "WriteBuffer must consume the latest version of the handle");
    AccumulateBufferUsage(node, access);

    const std::uint32_t newVersion = ++node.currentVersion;

    Internal::RGBufferAccessRecord record;
    record.bufferIndex      = handle.Index();
    record.versionConsumed  = handle.Version();
    record.versionProduced  = newVersion;
    record.isWrite          = true;
    record.access           = access;
    passes_[passIndex].bufferAccesses.push_back(record);

    auto& producers = bufferVersionProducers_[handle.Index()];
    assert(producers.size() == newVersion);
    producers.push_back(passIndex);

    return RGBufferHandle(handle.Index(), newVersion);
}

void RenderGraph::MarkPassSideEffect(std::uint32_t passIndex)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    passes_[passIndex].isSideEffect = true;
}

void RenderGraph::RecordColorAttachment(std::uint32_t   passIndex,
                                        std::uint32_t   slot,
                                        RGTextureHandle handle,
                                        RHI::LoadOp     loadOp,
                                        RHI::StoreOp    storeOp,
                                        RHI::ClearValue clearValue,
                                        RGTextureHandle resolveHandle)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());

    Internal::RGColorAttachmentRecord record{};
    record.slot                  = slot;
    record.textureIndex          = handle.Index();
    record.textureVersion        = handle.Version();
    record.loadOp                = loadOp;
    record.storeOp               = storeOp;
    record.clearValue            = clearValue;
    if (resolveHandle.IsValid())
    {
        record.hasResolve            = true;
        record.resolveTextureIndex   = resolveHandle.Index();
        record.resolveTextureVersion = resolveHandle.Version();
    }
    passes_[passIndex].colorAttachments.push_back(record);
}

void RenderGraph::RecordDepthAttachment(std::uint32_t   passIndex,
                                        RGTextureHandle handle,
                                        RHI::LoadOp     loadOp,
                                        RHI::StoreOp    storeOp,
                                        RHI::ClearValue clearValue)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    assert(!passes_[passIndex].depthAttachment.present
           && "Depth attachment already set for this pass");

    Internal::RGDepthAttachmentRecord& record = passes_[passIndex].depthAttachment;
    record.textureIndex    = handle.Index();
    record.textureVersion  = handle.Version();
    record.loadOp          = loadOp;
    record.storeOp         = storeOp;
    record.clearValue      = clearValue;
    record.present         = true;
}

void RenderGraph::RecordRenderArea(std::uint32_t passIndex, RHI::Rect2D area)
{
    assert(!compiled_);
    assert(passIndex < passes_.size());
    passes_[passIndex].renderArea    = area;
    passes_[passIndex].renderAreaSet = true;
}

void RenderGraph::AccumulateTextureUsage(Internal::RGTextureNode& node, const RGTextureAccess& access)
{
    node.inferredUsage |= InferTextureUsage(access);
}

void RenderGraph::AccumulateBufferUsage(Internal::RGBufferNode& node, const RGBufferAccess& access)
{
    node.inferredUsage |= InferBufferUsage(access);
}

void RenderGraph::CullDeadPasses()
{
    for (Internal::RGPassNode& pass : passes_)
    {
        pass.isAlive = pass.isSideEffect;
    }

    for (std::uint32_t texIndex = 0; texIndex < textures_.size(); ++texIndex)
    {
        if (!textures_[texIndex].finalLayout.has_value())
        {
            continue;
        }
        const auto& producers = textureVersionProducers_[texIndex];
        if (producers.size() > 1)
        {
            const std::uint32_t lastProducer = producers.back();
            if (lastProducer != kInvalidPassIndex)
            {
                passes_[lastProducer].isAlive = true;
            }
        }
    }

    for (std::size_t i = passes_.size(); i-- > 0; )
    {
        Internal::RGPassNode& pass = passes_[i];
        if (!pass.isAlive)
        {
            continue;
        }
        for (const Internal::RGTextureAccessRecord& record : pass.textureAccesses)
        {
            if (record.versionConsumed == 0)
            {
                continue;
            }
            const std::uint32_t producer =
                textureVersionProducers_[record.textureIndex][record.versionConsumed];
            if (producer != kInvalidPassIndex)
            {
                passes_[producer].isAlive = true;
            }
        }
        for (const Internal::RGBufferAccessRecord& record : pass.bufferAccesses)
        {
            if (record.versionConsumed == 0)
            {
                continue;
            }
            const std::uint32_t producer =
                bufferVersionProducers_[record.bufferIndex][record.versionConsumed];
            if (producer != kInvalidPassIndex)
            {
                passes_[producer].isAlive = true;
            }
        }
    }
}

void RenderGraph::TopologicallySortPasses()
{
    // User add-order is already a valid topological order: a pass can only reference handles
    // (and therefore versions produced by) passes added before it. We just keep the alive ones.
    executionOrder_.clear();
    executionOrder_.reserve(passes_.size());
    for (std::uint32_t i = 0; i < passes_.size(); ++i)
    {
        if (passes_[i].isAlive)
        {
            executionOrder_.push_back(i);
        }
    }
}

void RenderGraph::RealizeResources()
{
    for (Internal::RGTextureNode& node : textures_)
    {
        if (node.isImported)
        {
            node.realized = node.importedTexture;
            continue;
        }
        RHI::TextureDesc desc = node.desc;
        desc.usage |= node.inferredUsage;
        node.realized = transientPool_ != nullptr
                            ? transientPool_->AcquireTexture(desc, frameId_)
                            : device_->CreateTexture(desc);
        if (node.realized)
        {
            node.realized->SetDebugName(node.name);
        }
    }
    for (Internal::RGBufferNode& node : buffers_)
    {
        if (node.isImported)
        {
            node.realized = node.importedBuffer;
            continue;
        }
        RHI::BufferDesc desc = node.desc;
        desc.usage |= node.inferredUsage;
        node.realized = transientPool_ != nullptr
                            ? transientPool_->AcquireBuffer(desc, frameId_)
                            : device_->CreateBuffer(desc);
        if (node.realized)
        {
            node.realized->SetDebugName(node.name);
        }
    }
}

void RenderGraph::PlanBarriers()
{
    currentTextureState_.assign(textures_.size(), Internal::RGTextureState{});
    for (std::uint32_t i = 0; i < textures_.size(); ++i)
    {
        Internal::RGTextureState& state = currentTextureState_[i];
        state.layout = textures_[i].isImported ? textures_[i].initialLayout
                                                : RHI::ImageLayout::Undefined;
        state.stages = RHI::PipelineStageMask::TopOfPipe;
        state.access = RHI::AccessMask::None;
    }

    currentBufferState_.assign(buffers_.size(), Internal::RGBufferState{});

    for (std::uint32_t passIdx : executionOrder_)
    {
        Internal::RGPassNode& pass = passes_[passIdx];

        for (const Internal::RGTextureAccessRecord& record : pass.textureAccesses)
        {
            Internal::RGTextureState& prev = currentTextureState_[record.textureIndex];
            const bool layoutChange = prev.layout != record.access.layout;
            const bool prevWasWrite = IsWriteAccess(prev.access);
            const bool nextIsWrite  = record.isWrite;
            if (layoutChange || prevWasWrite || nextIsWrite)
            {
                RHI::TextureBarrier barrier{};
                barrier.texture   = textures_[record.textureIndex].realized.Get();
                barrier.range     = FullRange(textures_[record.textureIndex].desc);
                barrier.oldLayout = prev.layout;
                barrier.newLayout = record.access.layout;
                barrier.srcStages = prev.stages;
                barrier.srcAccess = prev.access;
                barrier.dstStages = record.access.stages;
                barrier.dstAccess = record.access.access;
                pass.preludeTextureBarriers.push_back(barrier);
            }
            prev.layout = record.access.layout;
            prev.stages = record.access.stages;
            prev.access = record.access.access;
        }

        for (const Internal::RGBufferAccessRecord& record : pass.bufferAccesses)
        {
            Internal::RGBufferState& prev = currentBufferState_[record.bufferIndex];
            const bool prevWasWrite = IsWriteAccess(prev.access);
            const bool nextIsWrite  = record.isWrite;
            if (prevWasWrite || nextIsWrite)
            {
                RHI::BufferBarrier barrier{};
                barrier.buffer    = buffers_[record.bufferIndex].realized.Get();
                barrier.offset    = 0;
                barrier.size      = RHI::kWholeSize;
                barrier.srcStages = prev.stages;
                barrier.srcAccess = prev.access;
                barrier.dstStages = record.access.stages;
                barrier.dstAccess = record.access.access;
                pass.preludeBufferBarriers.push_back(barrier);
            }
            prev.stages = record.access.stages;
            prev.access = record.access.access;
        }
    }
}

void RenderGraph::ComputeMergeHints()
{
    for (std::size_t i = 1; i < executionOrder_.size(); ++i)
    {
        Internal::RGPassNode& curr = passes_[executionOrder_[i]];
        Internal::RGPassNode& prev = passes_[executionOrder_[i - 1]];
        if (!curr.isRaster || !prev.isRaster)
        {
            continue;
        }
        auto* currRaster = static_cast<RGRasterPass*>(curr.pass);
        auto* prevRaster = static_cast<RGRasterPass*>(prev.pass);
        curr.mergeWithPrevious = currRaster->CanMergeWith(*prevRaster);
    }
}

void RenderGraph::Compile()
{
    assert(!compiled_ && "Compile called twice");
    CullDeadPasses();
    TopologicallySortPasses();
    RealizeResources();
    PlanBarriers();
    ComputeMergeHints();
    ComputeRenderPassBatches();
    compiled_ = true;
}

std::span<const Internal::RGRenderPassBatch> RenderGraph::RenderPassBatches() const noexcept
{
    return renderPassBatches_;
}

void RenderGraph::ComputeRenderPassBatches()
{
    renderPassBatches_.clear();
    if (executionOrder_.empty())
    {
        return;
    }

    auto deriveAttachments = [this](std::uint32_t passIdx) -> RHI::RHIRenderPassAttachments {
        const Internal::RGPassNode&   node = passes_[passIdx];
        RHI::RHIRenderPassAttachments att{};
        if (!node.isRaster)
        {
            return att;
        }
        std::uint32_t maxSlot     = 0;
        bool          hasAnyColor = false;
        std::uint32_t sampleCount = 1;
        for (const auto& ca : node.colorAttachments)
        {
            if (ca.slot >= RHI::kMaxColorAttachments)
            {
                continue;
            }
            const Internal::RGTextureNode& tex = textures_[ca.textureIndex];
            att.colorFormats[ca.slot] = tex.desc.format;
            if (!hasAnyColor || ca.slot > maxSlot)
            {
                maxSlot = ca.slot;
            }
            hasAnyColor = true;
            if (tex.desc.sampleCount > sampleCount)
            {
                sampleCount = tex.desc.sampleCount;
            }
        }
        att.colorAttachmentCount = hasAnyColor ? (maxSlot + 1) : 0;
        if (node.depthAttachment.present)
        {
            const Internal::RGTextureNode& tex = textures_[node.depthAttachment.textureIndex];
            att.depthFormat = tex.desc.format;
            if (tex.desc.sampleCount > sampleCount)
            {
                sampleCount = tex.desc.sampleCount;
            }
        }
        att.sampleCount = sampleCount;
        return att;
    };

    for (std::uint32_t i = 0; i < executionOrder_.size(); ++i)
    {
        const std::uint32_t          passIdx = executionOrder_[i];
        const Internal::RGPassNode&  node    = passes_[passIdx];

        const bool canExtend = !renderPassBatches_.empty() && node.isRaster && node.mergeWithPrevious;
        if (canExtend)
        {
            ++renderPassBatches_.back().count;
        }
        else
        {
            Internal::RGRenderPassBatch batch{};
            batch.firstExecIndex = i;
            batch.count          = 1;
            batch.attachments    = deriveAttachments(passIdx);
            renderPassBatches_.push_back(batch);
        }
    }
}

void RenderGraph::RecordPass(RHI::IRHICommandList& cmd, std::uint32_t compiledIndex)
{
    const std::uint32_t passIdx = executionOrder_[compiledIndex];
    Internal::RGPassNode& pass  = passes_[passIdx];

    cmd.PushDebugMarker(pass.name);

    if (!pass.preludeTextureBarriers.empty() || !pass.preludeBufferBarriers.empty())
    {
        RHI::BarrierGroup group;
        group.textures = std::span<const RHI::TextureBarrier>(pass.preludeTextureBarriers);
        group.buffers  = std::span<const RHI::BufferBarrier>(pass.preludeBufferBarriers);
        cmd.Barrier(group);
    }

    std::vector<RHI::RenderingAttachment> colorAttachments;
    std::optional<RHI::RenderingAttachment> depthAttachment;
    RHI::RenderingInfo renderingInfo{};

    if (pass.isRaster)
    {
        colorAttachments.reserve(pass.colorAttachments.size());
        for (const Internal::RGColorAttachmentRecord& a : pass.colorAttachments)
        {
            const RHI::TextureDesc& texDesc = textures_[a.textureIndex].desc;
            RHI::RenderingAttachment att{};
            att.view       = registry_->GetTextureView(a.textureIndex,
                                                       MakeAttachmentViewDesc(texDesc, false));
            att.layout     = RHI::ImageLayout::ColorAttachment;
            att.loadOp     = a.loadOp;
            att.storeOp    = a.storeOp;
            att.clearValue = a.clearValue;
            if (a.hasResolve)
            {
                const RHI::TextureDesc& resolveDesc = textures_[a.resolveTextureIndex].desc;
                att.resolveView   = registry_->GetTextureView(
                    a.resolveTextureIndex,
                    MakeAttachmentViewDesc(resolveDesc, false));
                att.resolveLayout = RHI::ImageLayout::ColorAttachment;
            }
            colorAttachments.push_back(att);
        }
        if (pass.depthAttachment.present)
        {
            const Internal::RGDepthAttachmentRecord& d = pass.depthAttachment;
            const RHI::TextureDesc& texDesc = textures_[d.textureIndex].desc;
            RHI::RenderingAttachment att{};
            att.view       = registry_->GetTextureView(d.textureIndex,
                                                       MakeAttachmentViewDesc(texDesc, true));
            att.layout     = RHI::ImageLayout::DepthStencilAttachment;
            att.loadOp     = d.loadOp;
            att.storeOp    = d.storeOp;
            att.clearValue = d.clearValue;
            depthAttachment = att;
        }

        RHI::Rect2D area = pass.renderArea;
        if (!pass.renderAreaSet)
        {
            const Internal::RGTextureNode* sizeSource = nullptr;
            if (!pass.colorAttachments.empty())
            {
                sizeSource = &textures_[pass.colorAttachments.front().textureIndex];
            }
            else if (pass.depthAttachment.present)
            {
                sizeSource = &textures_[pass.depthAttachment.textureIndex];
            }
            if (sizeSource != nullptr)
            {
                area.offset        = {0, 0};
                area.extent.width  = sizeSource->desc.extent.width;
                area.extent.height = sizeSource->desc.extent.height;
            }
        }

        renderingInfo.colorAttachments = std::span<const RHI::RenderingAttachment>(colorAttachments);
        renderingInfo.depthAttachment  = depthAttachment;
        renderingInfo.renderArea       = area;
        renderingInfo.layerCount       = 1;
        renderingInfo.viewMask         = 0;
        cmd.BeginRendering(renderingInfo);
    }

    RGContext context(registry_.get());
    pass.pass->Execute(context, cmd);

    if (pass.isRaster)
    {
        cmd.EndRendering();
    }

    cmd.PopDebugMarker();
}

void RenderGraph::Execute(RHI::IRHICommandList& cmd)
{
    assert(compiled_ && "Execute called before Compile");

    for (std::uint32_t i = 0; i < executionOrder_.size(); ++i)
    {
        RecordPass(cmd, i);
    }

    RecordFinalLayoutTransitions(cmd);
}

void RenderGraph::RecordFinalLayoutTransitions(RHI::IRHICommandList& cmd)
{
    std::vector<RHI::TextureBarrier> finalTextureBarriers;
    for (std::uint32_t texIdx = 0; texIdx < textures_.size(); ++texIdx)
    {
        const Internal::RGTextureNode& node = textures_[texIdx];
        if (!node.finalLayout.has_value() || !node.realized)
        {
            continue;
        }
        const Internal::RGTextureState& prev = currentTextureState_[texIdx];
        const RHI::ImageLayout finalLayout   = *node.finalLayout;
        if (prev.layout == finalLayout)
        {
            continue;
        }
        RHI::TextureBarrier barrier{};
        barrier.texture   = node.realized.Get();
        barrier.range     = FullRange(node.desc);
        barrier.oldLayout = prev.layout;
        barrier.newLayout = finalLayout;
        barrier.srcStages = prev.stages;
        barrier.srcAccess = prev.access;
        barrier.dstStages = RHI::PipelineStageMask::BottomOfPipe;
        barrier.dstAccess = RHI::AccessMask::None;
        finalTextureBarriers.push_back(barrier);
    }
    if (!finalTextureBarriers.empty())
    {
        RHI::BarrierGroup group;
        group.textures = std::span<const RHI::TextureBarrier>(finalTextureBarriers);
        cmd.Barrier(group);
    }
}

void RenderGraph::ExecuteParallel(const ParallelExecuteParams&             params,
                                  std::vector<Ref<RHI::IRHICommandList>>& outOrderedCmdLists)
{
    assert(compiled_ && "ExecuteParallel called before Compile");
    assert(params.acquireCmdList && "ParallelExecuteParams::acquireCmdList must be set");

    auto recordSerialBatch = [this, &params, &outOrderedCmdLists](const Internal::RGRenderPassBatch& batch) {
        Ref<RHI::IRHICommandList> cmd = params.acquireCmdList(-1);
        assert(cmd && "acquireCmdList returned null");
        cmd->Begin();
        for (std::uint32_t k = 0; k < batch.count; ++k)
        {
            RecordPass(*cmd, batch.firstExecIndex + k);
        }
        cmd->End();
        outOrderedCmdLists.push_back(std::move(cmd));
    };

    for (const Internal::RGRenderPassBatch& batch : renderPassBatches_)
    {
        const bool canParallelize = params.workerExec != nullptr && batch.count > 1;
        if (!canParallelize)
        {
            recordSerialBatch(batch);
            continue;
        }

        const std::uint32_t                    n = batch.count;
        std::vector<Ref<RHI::IRHICommandList>> batchCmds(n);

        TaskGraph g;
        for (std::uint32_t k = 0; k < n; ++k)
        {
            const std::uint32_t passCompiledIndex = batch.firstExecIndex + k;
            g.AddNodeOn(params.workerExec, "rg-pass", {},
                        [this, &params, &batchCmds, passCompiledIndex, k](TaskContext& ctx) {
                            Ref<RHI::IRHICommandList> cmd = params.acquireCmdList(ctx.WorkerIndex());
                            assert(cmd && "acquireCmdList returned null on worker");
                            cmd->Begin();
                            RecordPass(*cmd, passCompiledIndex);
                            cmd->End();
                            batchCmds[k] = std::move(cmd);
                        });
        }
        g.Submit(*params.workerExec).Wait();

        for (auto& cmd : batchCmds)
        {
            outOrderedCmdLists.push_back(std::move(cmd));
        }
    }

    Ref<RHI::IRHICommandList> finalCmd = params.acquireCmdList(-1);
    assert(finalCmd && "acquireCmdList returned null for final-layout-transition CL");
    finalCmd->Begin();
    RecordFinalLayoutTransitions(*finalCmd);
    finalCmd->End();
    outOrderedCmdLists.push_back(std::move(finalCmd));
}

} // namespace Hylux::RG
