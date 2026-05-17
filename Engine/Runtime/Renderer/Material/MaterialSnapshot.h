/// @file
/// @brief Cross-thread material payload. Game thread fills one from a MaterialInstance and
///        ships it through StructuralCommandQueue; render thread feeds it to
///        MaterialProxyCache for resolution. Carries pre-resolved bindless indices for
///        textures plus strong Refs to keep the underlying TextureAssets alive across the
///        thread boundary.

#pragma once

#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Self-contained material payload. No pointers into game-side memory; safe to copy
///        / move across threads. `referencedTextures` keeps every texture cited by
///        `textureBindlessIndices` alive until the render-thread MaterialProxy that
///        consumes it is destroyed.
struct MaterialSnapshot
{
    std::uint64_t                              materialAssetHash{0};
    std::uint64_t                              permutationKey{0};
    std::uint64_t                              instanceHash{0};
    std::vector<std::byte>                     uniformBlock{};
    std::vector<std::uint32_t>                 textureBindlessIndices{};
    std::vector<Ref<Asset::TextureAsset>>      referencedTextures{};
};

} // namespace Hylux::Renderer
