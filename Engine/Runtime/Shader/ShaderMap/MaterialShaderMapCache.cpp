/// @file
/// @brief MaterialShaderMapCache implementation. Constructs MaterialShaderMap on first
///        request and returns the same instance thereafter for a given material asset.

#include "Shader/ShaderMap/MaterialShaderMapCache.h"

namespace Hylux::Shader
{

MaterialShaderMap* MaterialShaderMapCache::GetOrLoad(std::uint64_t materialAssetHash)
{
    if (auto it = maps_.find(materialAssetHash); it != maps_.end())
    {
        return it->second.get();
    }
    auto map = std::make_unique<MaterialShaderMap>(materialAssetHash, archive_, moduleCache_);
    auto* raw = map.get();
    maps_.emplace(materialAssetHash, std::move(map));
    return raw;
}

} // namespace Hylux::Shader
