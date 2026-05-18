/// @file
/// @brief Builds a runtime TextureAsset from a cooked .hass payload. Returns a Future
///        that resolves to the constructed asset once the GPU upload completes (or to a
///        null Ref on parse / create / upload failure). Synchronous up to the upload
///        submit: the texture is created and staging is populated on the calling
///        thread, then the upload is enqueued on the AssetUploader's worker. When
///        ctx.uploader is null the texture is created but no pixels are uploaded — used
///        in unit tests that exercise the parse path only.

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/TextureAsset.h"
#include "Core/Async/Future.h"
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
    [[nodiscard]] static Future<Ref<TextureAsset>> Load(const Cooked::CookedReader& reader,
                                                        const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
