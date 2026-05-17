/// @file
/// @brief MaterialProxy implementation.

#include "Renderer/Material/MaterialProxy.h"

#include <utility>

namespace Hylux::Renderer
{

MaterialProxy::MaterialProxy(std::uint64_t                         assetHash,
                             std::uint64_t                         instanceHash,
                             std::uint64_t                         permutationKey,
                             Shader::MaterialShaderMap*            shaderMap,
                             std::uint32_t                         uniformBlockBindlessIndex,
                             Ref<RHI::IRHIBuffer>                  uniformBuffer,
                             std::vector<std::uint32_t>            textureBindlessIndices,
                             std::vector<Ref<Asset::TextureAsset>> referencedTextures) noexcept
    : assetHash_(assetHash),
      instanceHash_(instanceHash),
      permutationKey_(permutationKey),
      shaderMap_(shaderMap),
      uniformBlockBindlessIndex_(uniformBlockBindlessIndex),
      uniformBuffer_(std::move(uniformBuffer)),
      textureBindlessIndices_(std::move(textureBindlessIndices)),
      referencedTextures_(std::move(referencedTextures))
{
}

} // namespace Hylux::Renderer
