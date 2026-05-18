/// @file
/// @brief Sync2 / D3D12 enhanced-barrier compatible barrier descriptors.

#pragma once

#include "Core/Utils/Flags.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"

#include <cstdint>
#include <span>

namespace Hylux::RHI
{

/// @brief Bit flags for pipeline stages where a barrier source/destination is anchored.
///        Mirrors VkPipelineStageFlags2 and D3D12_BARRIER_SYNC bit semantics.
enum class PipelineStageMask : std::uint64_t
{
    None                       = 0,
    DrawIndirect               = 1ull << 0,
    VertexInput                = 1ull << 1,
    VertexShader               = 1ull << 2,
    HullShader                 = 1ull << 3,
    DomainShader               = 1ull << 4,
    GeometryShader             = 1ull << 5,
    PixelShader                = 1ull << 6,
    EarlyFragmentTests         = 1ull << 7,
    LateFragmentTests          = 1ull << 8,
    ColorAttachmentOutput      = 1ull << 9,
    ComputeShader              = 1ull << 10,
    TaskShader                 = 1ull << 11,
    MeshShader                 = 1ull << 12,
    RayTracingShader           = 1ull << 13,
    AccelerationStructureBuild = 1ull << 14,
    AccelerationStructureCopy  = 1ull << 15,
    Transfer                   = 1ull << 16,
    Resolve                    = 1ull << 17,
    Blit                       = 1ull << 18,
    Clear                      = 1ull << 19,
    Host                       = 1ull << 20,
    AllGraphics                = 1ull << 21,
    AllCommands                = 1ull << 22,
    TopOfPipe                  = 1ull << 23,
    BottomOfPipe               = 1ull << 24,
};

HYLUX_ENABLE_BITFLAGS(PipelineStageMask)

/// @brief Bit flags for memory access kinds, paired with PipelineStageMask in barriers.
///        Mirrors VkAccessFlags2 and D3D12_BARRIER_ACCESS bit semantics.
enum class AccessMask : std::uint64_t
{
    None                          = 0,
    IndirectArgRead               = 1ull << 0,
    IndexBufferRead               = 1ull << 1,
    VertexBufferRead              = 1ull << 2,
    UniformRead                   = 1ull << 3,
    ShaderResourceRead            = 1ull << 4,
    ShaderResourceWrite           = 1ull << 5,
    ColorAttachmentRead           = 1ull << 6,
    ColorAttachmentWrite          = 1ull << 7,
    DepthStencilAttachmentRead    = 1ull << 8,
    DepthStencilAttachmentWrite   = 1ull << 9,
    TransferRead                  = 1ull << 10,
    TransferWrite                 = 1ull << 11,
    HostRead                      = 1ull << 12,
    HostWrite                     = 1ull << 13,
    AccelerationStructureRead     = 1ull << 14,
    AccelerationStructureWrite    = 1ull << 15,
    ShadingRateRead               = 1ull << 16,
    Present                       = 1ull << 17,
    MemoryRead                    = 1ull << 18,
    MemoryWrite                   = 1ull << 19,
    InputAttachmentRead           = 1ull << 20,
};

HYLUX_ENABLE_BITFLAGS(AccessMask)

/// @brief Reserved queue family index used when no queue ownership transfer is performed.
inline constexpr std::uint32_t kQueueFamilyIgnored = 0xFFFFFFFFu;

/// @brief Global memory barrier: synchronizes all matching memory accesses without referring
///        to a specific resource.
struct MemoryBarrier
{
    PipelineStageMask srcStages{PipelineStageMask::None};
    AccessMask        srcAccess{AccessMask::None};
    PipelineStageMask dstStages{PipelineStageMask::None};
    AccessMask        dstAccess{AccessMask::None};
};

/// @brief Range-scoped buffer barrier. Synchronizes accesses and optionally transfers queue
///        ownership of a sub-range of the buffer.
struct BufferBarrier
{
    IRHIBuffer*       buffer{nullptr};
    std::uint64_t     offset{0};
    std::uint64_t     size{kWholeSize};
    PipelineStageMask srcStages{PipelineStageMask::None};
    AccessMask        srcAccess{AccessMask::None};
    PipelineStageMask dstStages{PipelineStageMask::None};
    AccessMask        dstAccess{AccessMask::None};
    std::uint32_t     srcQueueFamilyIndex{kQueueFamilyIgnored};
    std::uint32_t     dstQueueFamilyIndex{kQueueFamilyIgnored};
};

/// @brief Subresource-scoped texture barrier. Performs sync + layout transition + optional
///        queue ownership transfer.
struct TextureBarrier
{
    IRHITexture*      texture{nullptr};
    SubresourceRange  range{};
    ImageLayout       oldLayout{ImageLayout::Undefined};
    ImageLayout       newLayout{ImageLayout::Undefined};
    PipelineStageMask srcStages{PipelineStageMask::None};
    AccessMask        srcAccess{AccessMask::None};
    PipelineStageMask dstStages{PipelineStageMask::None};
    AccessMask        dstAccess{AccessMask::None};
    std::uint32_t     srcQueueFamilyIndex{kQueueFamilyIgnored};
    std::uint32_t     dstQueueFamilyIndex{kQueueFamilyIgnored};
};

/// @brief Aggregate barrier batch submitted to IRHICommandList::Barrier in a single call.
struct BarrierGroup
{
    std::span<const MemoryBarrier>  memory{};
    std::span<const BufferBarrier>  buffers{};
    std::span<const TextureBarrier> textures{};
};

} // namespace Hylux::RHI
