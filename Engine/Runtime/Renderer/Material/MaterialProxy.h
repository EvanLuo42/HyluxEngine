/// @file
/// @brief Render-thread mirror of a MaterialInstance. Built once per
///        (assetHash, instanceHash) pair by MaterialProxyCache and reused across every
///        primitive that asks for the same combination.

#pragma once

#include "Shader/ShaderMap/MaterialShaderMap.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Immutable render-side material record. Built lazily on the render thread when a
///        new MaterialSnapshot arrives. Pointer stable for the proxy's lifetime; cache
///        ownership guarantees the proxy outlives any PrimitiveProxy referring to it.
class MaterialProxy
{
public:
    MaterialProxy(std::uint64_t              assetHash,
                  std::uint64_t              instanceHash,
                  std::uint64_t              permutationKey,
                  Shader::MaterialShaderMap* shaderMap,
                  std::uint32_t              uniformBlockBindlessIndex,
                  std::vector<std::uint32_t> textureBindlessIndices) noexcept;

    [[nodiscard]] std::uint64_t                  GetMaterialAssetHash() const noexcept { return assetHash_; }
    [[nodiscard]] std::uint64_t                  GetInstanceHash() const noexcept { return instanceHash_; }
    [[nodiscard]] std::uint64_t                  GetPermutationKey() const noexcept { return permutationKey_; }
    [[nodiscard]] Shader::MaterialShaderMap*     GetShaderMap() const noexcept { return shaderMap_; }
    [[nodiscard]] std::uint32_t                  GetUniformBlockBindlessIndex() const noexcept { return uniformBlockBindlessIndex_; }
    [[nodiscard]] std::span<const std::uint32_t> GetTextureBindlessIndices() const noexcept { return textureBindlessIndices_; }

private:
    std::uint64_t              assetHash_{0};
    std::uint64_t              instanceHash_{0};
    std::uint64_t              permutationKey_{0};
    Shader::MaterialShaderMap* shaderMap_{nullptr};
    std::uint32_t              uniformBlockBindlessIndex_{0xFFFFFFFFu};
    std::vector<std::uint32_t> textureBindlessIndices_{};
};

} // namespace Hylux::Renderer
