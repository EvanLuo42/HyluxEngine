/// @file
/// @brief Builds a runtime MeshAsset from a cooked .hass payload. Allocates GPU vertex
///        + index buffers and stages their contents via ctx.uploader. If ctx.uploader
///        is null, buffers are still created but left empty (unit-test convenience).

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/MeshAsset.h"
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
    [[nodiscard]] static Ref<MeshAsset> Load(const Cooked::CookedReader& reader,
                                              const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
