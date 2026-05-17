/// @file
/// @brief Builds a runtime MaterialInstanceAsset from a cooked .hass payload. Recurses
///        through ctx.subsystem to load the parent MaterialAsset and any TextureAsset
///        override targets.

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/MaterialInstanceAsset.h"
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
    [[nodiscard]] static Ref<MaterialInstanceAsset> Load(const Cooked::CookedReader& reader,
                                                         const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
