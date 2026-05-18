/// @file
/// @brief Builds a runtime MaterialInstanceAsset from a cooked .hass payload. Recurses
///        through ctx.subsystem to load the parent MaterialAsset asynchronously and
///        chains the instance construction on that future. Texture override targets are
///        stored as SoftAssetRefs in the asset; they are resolved lazily by the
///        consuming MaterialInstance.

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/MaterialInstanceAsset.h"
#include "Core/Async/Future.h"
#include "Core/Memory/Ref.h"

namespace Hylux::Asset::Cooked
{
class CookedReader;
}

namespace Hylux::Asset
{

class MaterialInstanceLoader
{
public:
    [[nodiscard]] static Future<Ref<MaterialInstanceAsset>> Load(const Cooked::CookedReader& reader,
                                                                 const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
