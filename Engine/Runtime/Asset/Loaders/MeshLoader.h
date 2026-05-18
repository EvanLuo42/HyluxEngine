/// @file
/// @brief Builds a runtime MeshAsset from a cooked .hass payload. Returns a Future that
///        resolves once the GPU buffer uploads complete. Dispatches on
///        Cooked::MeshPayload::kind to produce either a PhysicalMeshAsset (allocates GPU
///        vertex + index buffers and stages their contents via ctx.uploader) or a
///        VirtualMeshAsset (cluster-page streamed; not yet implemented in v1). If
///        ctx.uploader is null, physical buffers are still created but left empty and
///        the future resolves immediately (unit-test convenience).

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/MeshAsset.h"
#include "Core/Async/Future.h"
#include "Core/Memory/Ref.h"

namespace Hylux::Asset::Cooked
{
class CookedReader;
}

namespace Hylux::Asset
{

class MeshLoader
{
public:
    [[nodiscard]] static Future<Ref<MeshAsset>> Load(const Cooked::CookedReader& reader,
                                                     const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
