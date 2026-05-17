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
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/RGAccess.h"

#include <cstdint>
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
struct RGPassNode
{
    std::string                          name;
    RGPass*                              pass{nullptr};
    bool                                 isRaster{false};
    bool                                 isSideEffect{false};
    bool                                 isAlive{true};

    std::vector<RGTextureAccessRecord>   textureAccesses;
    std::vector<RGBufferAccessRecord>    bufferAccesses;

    std::vector<RGColorAttachmentRecord> colorAttachments;
    RGDepthAttachmentRecord              depthAttachment{};
    RHI::Rect2D                          renderArea{};
    bool                                 renderAreaSet{false};

    std::vector<RHI::TextureBarrier>     preludeTextureBarriers;
    std::vector<RHI::BufferBarrier>      preludeBufferBarriers;
};

} // namespace Internal
} // namespace Hylux::RG
