/// @file
/// @brief Render-thread mirror of a MaterialInstance. Built once per
///        (assetHash, instanceHash) pair by MaterialProxyCache and reused across every
///        primitive that asks for the same combination.

#pragma once

#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "Shader/ShaderMap/MaterialShaderMap.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Immutable render-side material record. Built lazily on the render thread when a
///        new MaterialSnapshot arrives. Pointer stable for the proxy's lifetime; cache
///        ownership guarantees the proxy outlives any PrimitiveProxy referring to it.
///        Owns the per-instance uniform buffer (mapped CpuToGpu) and strong Refs to every
///        TextureAsset whose bindless index it references — both stay alive as long as the
///        proxy does, so dropping a game-side MaterialInstance cannot invalidate the GPU
///        bindings the renderer reads.
class MaterialProxy
{
public:
    MaterialProxy(std::uint64_t                         assetHash,
                  std::uint64_t                         instanceHash,
                  std::uint64_t                         permutationKey,
                  Shader::MaterialShaderMap*            shaderMap,
                  std::uint32_t                         uniformBlockBindlessIndex,
                  Ref<RHI::IRHIBuffer>                  uniformBuffer,
                  std::vector<std::uint32_t>            textureBindlessIndices,
                  std::vector<Ref<Asset::TextureAsset>> referencedTextures) noexcept;

    [[nodiscard]] std::uint64_t                  GetMaterialAssetHash() const noexcept { return assetHash_; }
    [[nodiscard]] std::uint64_t                  GetInstanceHash() const noexcept { return instanceHash_; }
    [[nodiscard]] std::uint64_t                  GetPermutationKey() const noexcept { return permutationKey_; }
    [[nodiscard]] Shader::MaterialShaderMap*     GetShaderMap() const noexcept { return shaderMap_; }
    [[nodiscard]] std::uint32_t                  GetUniformBlockBindlessIndex() const noexcept { return uniformBlockBindlessIndex_; }
    [[nodiscard]] RHI::IRHIBuffer*               GetUniformBuffer() const noexcept { return uniformBuffer_.Get(); }
    [[nodiscard]] std::span<const std::uint32_t> GetTextureBindlessIndices() const noexcept { return textureBindlessIndices_; }

private:
    std::uint64_t                         assetHash_{0};
    std::uint64_t                         instanceHash_{0};
    std::uint64_t                         permutationKey_{0};
    Shader::MaterialShaderMap*            shaderMap_{nullptr};
    std::uint32_t                         uniformBlockBindlessIndex_{0xFFFFFFFFu};
    Ref<RHI::IRHIBuffer>                  uniformBuffer_;
    std::vector<std::uint32_t>            textureBindlessIndices_;
    std::vector<Ref<Asset::TextureAsset>> referencedTextures_;
};

} // namespace Hylux::Renderer
