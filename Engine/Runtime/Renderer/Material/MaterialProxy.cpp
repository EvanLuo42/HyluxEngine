/// @file
/// @brief MaterialProxy implementation.

#include "Renderer/Material/MaterialProxy.h"

#include <utility>

namespace Hylux::Renderer
{

MaterialProxy::MaterialProxy(std::uint64_t              assetHash,
                             std::uint64_t              instanceHash,
                             std::uint64_t              permutationKey,
                             Shader::MaterialShaderMap* shaderMap,
                             std::uint32_t              uniformBlockBindlessIndex,
                             std::vector<std::uint32_t> textureBindlessIndices) noexcept
    : assetHash_(assetHash),
      instanceHash_(instanceHash),
      permutationKey_(permutationKey),
      shaderMap_(shaderMap),
      uniformBlockBindlessIndex_(uniformBlockBindlessIndex),
      textureBindlessIndices_(std::move(textureBindlessIndices))
{
}

} // namespace Hylux::Renderer
