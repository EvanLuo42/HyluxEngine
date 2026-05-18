/// @file
/// @brief Internal node structures owned by RenderGraph: per-resource state and per-pass
///        recorded reads/writes/attachments. Not part of the public RG API.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIPipelineDesc.h"
#include "RHI/RHIRenderPassAttachments.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGPass.h"

#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string>
#include <vector>

namespace Hylux::RG
{

class RGPass;

namespace Internal
{

/// @brief Combined sync state assigned to a resource between consecutive uses. The graph
///        compares the previous and current snapshots to emit barriers.
struct RGTextureState
{
    RHI::ImageLayout       layout{RHI::ImageLayout::Undefined};
    RHI::PipelineStageMask stages{RHI::PipelineStageMask::TopOfPipe};
    RHI::AccessMask        access{RHI::AccessMask::None};
};

struct RGBufferState
{
    RHI::PipelineStageMask stages{RHI::PipelineStageMask::TopOfPipe};
    RHI::AccessMask        access{RHI::AccessMask::None};
};

/// @brief Internal record of a single texture node. Aggregates the descriptor, import state,
///        version counter, and the realized GPU resource once compiled.
struct RGTextureNode
{
    std::string                       name;
    RHI::TextureDesc                  desc{};
    bool                              isImported{false};
    Ref<RHI::IRHITexture>             importedTexture;
    RHI::ImageLayout                  initialLayout{RHI::ImageLayout::Undefined};
    std::optional<RHI::ImageLayout>   finalLayout;
    std::uint32_t                     currentVersion{0};
    RHI::TextureUsage                 inferredUsage{RHI::TextureUsage::None};
    Ref<RHI::IRHITexture>             realized;
};

struct RGBufferNode
{
    std::string                       name;
    RHI::BufferDesc                   desc{};
    bool                              isImported{false};
    Ref<RHI::IRHIBuffer>              importedBuffer;
    std::uint32_t                     currentVersion{0};
    RHI::BufferUsage                  inferredUsage{RHI::BufferUsage::None};
    Ref<RHI::IRHIBuffer>              realized;
};

/// @brief One read or write of a texture by a pass. Writes record the version the pass produces.
struct RGTextureAccessRecord
{
    std::uint32_t   textureIndex{0};
    std::uint32_t   versionConsumed{0};   // for reads / for writes: the input version
    std::uint32_t   versionProduced{0};   // writes only; 0 on reads
    bool            isWrite{false};
    RGTextureAccess access{};
};

struct RGBufferAccessRecord
{
    std::uint32_t  bufferIndex{0};
    std::uint32_t  versionConsumed{0};
    std::uint32_t  versionProduced{0};
    bool           isWrite{false};
    RGBufferAccess access{};
};

/// @brief Render attachment record produced by RGRasterBuilder. The texture is identified by
///        the handle's (index, version) pair; the corresponding write is already entered in
///        textureAccesses_ by the builder.
struct RGColorAttachmentRecord
{
    std::uint32_t   slot{0};
    std::uint32_t   textureIndex{0};
    std::uint32_t   textureVersion{0};
    std::uint32_t   resolveTextureIndex{0};
    std::uint32_t   resolveTextureVersion{0};
    bool            hasResolve{false};
    RHI::LoadOp     loadOp{RHI::LoadOp::Load};
    RHI::StoreOp    storeOp{RHI::StoreOp::Store};
    RHI::ClearValue clearValue{};
};

struct RGDepthAttachmentRecord
{
    std::uint32_t   textureIndex{0};
    std::uint32_t   textureVersion{0};
    RHI::LoadOp     loadOp{RHI::LoadOp::Load};
    RHI::StoreOp    storeOp{RHI::StoreOp::Store};
    RHI::ClearValue clearValue{};
    bool            present{false};
};

/// @brief Internal record of a pass: the owning class pointer plus everything Setup recorded
///        through its builder. Barriers are populated during Compile.
///
///        Every per-pass list lives inside a std::pmr::vector so the owning RenderGraph can
///        thread its frame arena down. The default constructor falls back to the default
///        heap resource; the explicit-resource constructor is the one AddPass uses.
struct RGPassNode
{
    RGPassNode() noexcept = default;

    explicit RGPassNode(std::pmr::memory_resource* resource) noexcept
        : textureAccesses(resource),
          bufferAccesses(resource),
          colorAttachments(resource),
          preludeTextureBarriers(resource),
          preludeBufferBarriers(resource)
    {}

    std::string                               name;
    RGPass*                                   pass{nullptr};
    RGPassKind                                kind{RGPassKind::Generic};
    QueueAffinity                             queueAffinity{QueueAffinity::Graphics};
    bool                                      isRaster{false};
    bool                                      isSideEffect{false};
    bool                                      isAlive{true};

    /// @brief Subpass-merge hint computed during Compile. When true, this raster pass would
    ///        prefer to share its render-pass instance with the previous compiled pass so the
    ///        backend can keep attachments in tile memory between them. The current executor
    ///        does not yet act on this — barrier planning and dynamic-rendering wrap-up still
    ///        treat every raster pass as a standalone render pass. Reserved so the eventual
    ///        merger can plug in without touching the public RGRasterBuilder surface.
    bool                                      mergeWithPrevious{false};

    std::pmr::vector<RGTextureAccessRecord>   textureAccesses;
    std::pmr::vector<RGBufferAccessRecord>    bufferAccesses;

    std::pmr::vector<RGColorAttachmentRecord> colorAttachments;
    RGDepthAttachmentRecord                   depthAttachment{};
    RHI::Rect2D                               renderArea{};
    bool                                      renderAreaSet{false};

    std::pmr::vector<RHI::TextureBarrier>     preludeTextureBarriers;
    std::pmr::vector<RHI::BufferBarrier>      preludeBufferBarriers;
};

/// @brief Compile-time grouping of consecutive raster passes that share a renderpass
///        instance. Produced by RenderGraph::ComputeRenderPassBatches and consumed by both
///        the serial and parallel Execute paths so neither has to re-derive batching.
///        firstExecIndex / count index into RenderGraph::executionOrder_. A batch with
///        count == 1 is the trivial case (one pass, its own renderpass instance); count > 1
///        means every member raster pass shares the same RHIRenderPassAttachments.
struct RGRenderPassBatch
{
    std::uint32_t                  firstExecIndex{0};
    std::uint32_t                  count{0};
    RHI::RHIRenderPassAttachments  attachments{};
};

} // namespace Internal
} // namespace Hylux::RG
