/// @file
/// @brief MaterialProxyCache implementation.

#include "Renderer/Material/MaterialProxyCache.h"

#include "Shader/ShaderSubsystem.h"

namespace Hylux::Renderer
{

MaterialProxyCache::MaterialProxyCache(Shader::ShaderSubsystem* shaders) noexcept : shaders_(shaders) {}

MaterialProxy* MaterialProxyCache::GetOrCreate(const MaterialSnapshot& snapshot)
{
    const Key key{snapshot.materialAssetHash, snapshot.instanceHash};
    if (auto it = entries_.find(key); it != entries_.end())
    {
        return it->second.get();
    }

    Shader::MaterialShaderMap* shaderMap = nullptr;
    if (shaders_ != nullptr && snapshot.materialAssetHash != 0)
    {
        shaderMap = shaders_->GetOrLoadMaterialShaderMap(snapshot.materialAssetHash);
    }

    // Stage 6: uniform block upload + per-texture bindless resolution land with
    // UploadHeapManager and the asset system. Both index fields stay at Invalid sentinel.
    auto proxy = std::make_unique<MaterialProxy>(snapshot.materialAssetHash,
                                                 snapshot.instanceHash,
                                                 snapshot.permutationKey,
                                                 shaderMap,
                                                 /*uniformBlockBindlessIndex*/ 0xFFFFFFFFu,
                                                 /*textureBindlessIndices*/ std::vector<std::uint32_t>{});

    auto* raw = proxy.get();
    entries_.emplace(key, std::move(proxy));
    return raw;
}

void MaterialProxyCache::Clear() noexcept
{
    entries_.clear();
}

} // namespace Hylux::Renderer
