/// @file
/// @brief AssetLoaderContext + IAssetUploader — the bundle of dependencies a per-type
///        loader needs to construct a runtime asset from a cooked .hass payload.
///        Decouples loaders from AssetSubsystem so they remain unit-testable.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetTypeId.h"
#include "Asset/Cooked/CookedFormat.h"
#include "Core/Guid.h"
#include "Core/Async/Future.h"
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

/// @brief Interface that loaders use to push CPU bytes into GPU resources. The
///        AssetSubsystem-owned implementation creates a transient staging buffer,
///        records the copy, submits on the graphics queue, and blocks on a fence
///        (v1 sync). v2 reroutes to a transfer queue + worker thread without
///        changing this surface.
class IAssetUploader
{
public:
    virtual ~IAssetUploader() = default;

    /// @brief Copies @p bytes into the front of @p destination via staging. Returns
    ///        false on failure (typically OOM on the staging side).
    virtual bool UploadBufferData(RHI::IRHIBuffer&           destination,
                                  const void*                bytes,
                                  std::uint64_t              size) = 0;

    /// @brief Copies a series of mip slices into @p destination, transitioning the
    ///        texture to ShaderReadOnly when done. Returns false on failure.
    virtual bool UploadTextureMips(RHI::IRHITexture&         destination,
                                   std::span<const MipUpload> mips) = 0;
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
