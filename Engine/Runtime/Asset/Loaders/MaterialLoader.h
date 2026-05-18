/// @file
/// @brief Builds a runtime MaterialAsset from a cooked .hass payload. Pure CPU work
///        plus a single shader-map lookup; no GPU upload — so the returned Future is
///        always already-ready (MakeReady). Kept Future-shaped for uniformity with the
///        other loaders.

#pragma once

#include "Asset/AssetLoaderContext.h"
#include "Asset/Types/MaterialAsset.h"
#include "Core/Async/Future.h"
#include "Core/Memory/Ref.h"

namespace Hylux::Asset::Cooked
{
class CookedReader;
}

namespace Hylux::Asset
{

class MaterialLoader
{
public:
    /// @brief Parses the reader's payload + ref table, resolves the shader map via the
    ///        context's ShaderSubsystem, and constructs a MaterialAsset. The future
    ///        resolves to null on any malformed input.
    [[nodiscard]] static Future<Ref<MaterialAsset>> Load(const Cooked::CookedReader& reader,
                                                         const AssetLoaderContext&    ctx);
};

} // namespace Hylux::Asset
