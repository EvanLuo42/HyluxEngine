/// @file
/// @brief Builds a runtime TextureAsset from a cooked .hass payload. Creates a single
///        2D texture with the declared mip chain and stages per-mip pixel bytes through
///        ctx.uploader. When ctx.uploader is null the texture is created but no pixels
///        are uploaded — used in unit tests that exercise the parse path only.

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"

namespace Hylux::Asset::Cooked
{
class CookedReader;
}

namespace Hylux::Asset
{

class TextureLoader
{
public:
    [[nodiscard]] static Ref<TextureAsset> Load(const Cooked::CookedReader& reader,
                                                 const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
