/// @file
/// @brief Cross-thread material payload. Game thread fills one from a MaterialInstance and
///        ships it through StructuralCommandQueue; render thread feeds it to
///        MaterialProxyCache for resolution.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Self-contained material payload. No pointers into game-side memory; safe to copy
///        / move across threads.
struct MaterialSnapshot
{
    std::uint64_t              materialAssetHash{0};
    std::uint64_t              permutationKey{0};
    std::uint64_t              instanceHash{0};
    std::vector<std::byte>     uniformBlock{};
    std::vector<std::uint64_t> textureHandles{};
};

} // namespace Hylux::Renderer
