/// @file
/// @brief AssetLoaderContext + IAssetUploader — the bundle of dependencies a per-type
///        loader needs to construct a runtime asset from a cooked .hass payload.
///        Decouples loaders from AssetSubsystem so they remain unit-testable.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetTypeId.h"
#include "Asset/Cooked/CookedFormat.h"
#include "Core/Async/CancellationToken.h"
#include "Core/Async/Future.h"
#include "Core/Guid.h"
#include "Core/Memory/Ref.h"
#include "RHI/RHIForward.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace Hylux
{
class IVirtualFileSystem;
}

namespace Hylux::Shader
{
class ShaderSubsystem;
}

namespace Hylux::Asset
{

class AssetSubsystem;
class AssetRegistry;

/// @brief Single mip slice to be uploaded into a texture. `bytes` points into the source
///        buffer (typically the CookedReader's mmap-style buffer) and stays valid only
///        for the duration of the upload call.
struct MipUpload
{
    std::uint32_t            mipLevel{0};
    std::uint32_t            arrayLayer{0};
    std::uint32_t            width{0};
    std::uint32_t            height{0};
    std::span<const std::byte> bytes{};
};

/// @brief One sparse-tile slab to be uploaded into a virtual / sparse texture. Coordinates
///        are in *tile units* (not pixels), matching the sparse-binding granularity the
///        RHI reports for the texture's format. `bytes` follows the same lifetime rule as
///        MipUpload::bytes. Used by VirtualTextureAsset's residency manager once the VT
///        backend lands.
struct TileUpload
{
    std::uint32_t            mipLevel{0};
    std::uint32_t            arrayLayer{0};
    std::uint32_t            tileX{0};
    std::uint32_t            tileY{0};
    std::uint32_t            tileZ{0};
    std::uint32_t            tileCountX{1};
    std::uint32_t            tileCountY{1};
    std::uint32_t            tileCountZ{1};
    std::span<const std::byte> bytes{};
};

/// @brief Interface that loaders use to push CPU bytes into GPU resources. Every entry
///        point returns a Future<bool> that resolves true on success, false on failure
///        (cancellation, staging OOM, queue submission failure). The
///        AssetSubsystem-owned implementation allocates the staging buffer on the
///        caller's thread (so the source bytes can be released immediately) and pushes
///        the cmd-buffer recording + submit + fence-wait to a dedicated worker thread,
///        firing the Promise from that worker. The Tile / Range entry points are stubs
///        in v1 — VirtualTextureAsset and VirtualMeshAsset will start exercising them
///        once their backends land.
class IAssetUploader
{
public:
    virtual ~IAssetUploader() = default;

    /// @brief Copies @p bytes into the front of @p destination via staging. Returns a
    ///        Future that resolves once the GPU copy completes.
    [[nodiscard]] virtual Future<bool> UploadBufferData(RHI::IRHIBuffer&  destination,
                                                        const void*       bytes,
                                                        std::uint64_t     size,
                                                        CancellationToken token = {}) = 0;

    /// @brief Copies @p size bytes into @p destination starting at @p dstOffset. Used by
    ///        page-streamed assets (VirtualMeshAsset cluster pages, GPU residency tables)
    ///        that update a sub-range of a long-lived buffer instead of replacing the
    ///        whole resource. Resolves false if the implementation does not yet support
    ///        ranged uploads.
    [[nodiscard]] virtual Future<bool> UploadBufferRange(RHI::IRHIBuffer&  destination,
                                                         std::uint64_t     dstOffset,
                                                         const void*       bytes,
                                                         std::uint64_t     size,
                                                         CancellationToken token = {}) = 0;

    /// @brief Copies a series of mip slices into @p destination, transitioning the
    ///        texture to ShaderReadOnly when done.
    [[nodiscard]] virtual Future<bool> UploadTextureMips(RHI::IRHITexture&          destination,
                                                         std::span<const MipUpload>  mips,
                                                         CancellationToken          token = {}) = 0;

    /// @brief Copies a series of sparse tiles into @p destination. Expects @p destination
    ///        to have been created with TextureDesc::sparse = true; the implementation
    ///        binds backing memory for the requested tiles (sparse-image bind) before
    ///        issuing the buffer-to-image copy. Resolves false if the implementation
    ///        does not yet support sparse uploads.
    [[nodiscard]] virtual Future<bool> UploadTextureTiles(RHI::IRHITexture&            destination,
                                                          std::span<const TileUpload>  tiles,
                                                          CancellationToken            token = {}) = 0;
};

/// @brief Context handed to per-type loaders. All pointers are non-owning and outlive
///        the loader call. `uploader` may be null in unit-test contexts where loaders
///        are exercised without a real GPU; in production the AssetSubsystem always
///        supplies a real one. `subsystem` lets loaders recurse for dependent assets
///        (e.g. MaterialInstanceLoader → parent MaterialAsset).
struct AssetLoaderContext
{
    IVirtualFileSystem*      vfs{nullptr};
    RHI::IRHIDevice*         device{nullptr};
    Shader::ShaderSubsystem* shaders{nullptr};
    IAssetUploader*          uploader{nullptr};
    AssetSubsystem*          subsystem{nullptr};
    const AssetRegistry*     registry{nullptr};
};

} // namespace Hylux::Asset
