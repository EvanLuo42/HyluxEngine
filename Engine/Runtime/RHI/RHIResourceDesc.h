/// @file
/// @brief Descriptor structs for buffer, texture, sampler, and view creation.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Strongly typed bindless table slot. The Invalid sentinel signifies that a
///        resource was created with the corresponding bindless flag disabled.
enum class BindlessIndex : std::uint32_t
{
    Invalid = 0xFFFFFFFFu,
};

/// @brief Description used to create an IRHIBuffer. Bindless flags request the device to
///        allocate the appropriate descriptor in the SrvCbvUav heap on creation. `sparse`
///        requests a sparse / tiled buffer whose backing pages are bound on demand by the
///        owner (e.g. a streamed virtual-mesh cluster pool); the backend creates the
///        resource with no resident memory until the application binds pages explicitly.
struct BufferDesc
{
    std::uint64_t size{0};
    BufferUsage   usage{BufferUsage::None};
    MemoryUsage   memory{MemoryUsage::GpuOnly};
    std::uint32_t structureStride{0};
    bool          bindlessSrv{false};
    bool          bindlessUav{false};
    bool          bindlessCbv{false};
    bool          aliasable{false};
    bool          sparse{false};
};

/// @brief Description used to create an IRHITexture. `sparse` requests a sparse / tiled
///        image whose backing tiles are bound on demand by the owner (e.g. a virtual
///        texture's tile pool); the backend creates the resource with no resident tiles
///        until the application binds them through the uploader's tile path.
struct TextureDesc
{
    TextureDimension dimension{TextureDimension::Tex2D};
    Format           format{Format::Unknown};
    Extent3D         extent{};
    std::uint32_t    mipLevels{1};
    std::uint32_t    arrayLayers{1};
    std::uint32_t    sampleCount{1};
    TextureUsage     usage{TextureUsage::None};
    MemoryUsage     memory{MemoryUsage::GpuOnly};
    ClearValue       optimalClear{};
    bool             aliasable{false};
    bool             sparse{false};
};

/// @brief Description used to create an IRHITextureView. A default view describes the entire
///        resource; specifying explicit subresource ranges yields targeted views.
struct TextureViewDesc
{
    TextureDimension dimension{TextureDimension::Tex2D};
    Format           format{Format::Unknown};
    SubresourceRange range{};
    bool             isShaderResourceView{true};
    bool             isUnorderedAccessView{false};
    bool             isRenderTargetView{false};
    bool             isDepthStencilView{false};
};

/// @brief Description used to create an IRHISampler. Bindless allocation is implicit since
///        the sampler heap is small enough to always track in the bindless table.
struct SamplerDesc
{
    FilterMode    minFilter{FilterMode::Linear};
    FilterMode    magFilter{FilterMode::Linear};
    MipFilterMode mipFilter{MipFilterMode::Linear};
    AddressMode   addressU{AddressMode::Repeat};
    AddressMode   addressV{AddressMode::Repeat};
    AddressMode   addressW{AddressMode::Repeat};
    float         mipLodBias{0.0f};
    float         minLod{0.0f};
    float         maxLod{1000.0f};
    float         maxAnisotropy{1.0f};
    bool          compareEnable{false};
    CompareOp     compareOp{CompareOp::Never};
    float         borderColor[4]{0.0f, 0.0f, 0.0f, 0.0f};
};

/// @brief Region descriptor identifying a sub-rectangle of a texture for copy/clear/resolve.
struct TextureRegion
{
    SubresourceRange range{};
    Offset3D         offset{};
    Extent3D         extent{};
};

/// @brief Layout descriptor mapping a buffer range to a tightly packed image region for
///        buffer/texture copies. Rows and slices follow Vulkan/D3D12 conventions.
struct BufferTextureLayout
{
    std::uint64_t offset{0};
    std::uint32_t rowPitch{0};
    std::uint32_t slicePitch{0};
};

/// @brief Description for IRHICommandList::ResolveTexture.
struct ResolveDesc
{
    SubresourceRange srcRange{};
    SubresourceRange dstRange{};
};

} // namespace Hylux::RHI
