/// @file
/// @brief MaterialShaderMap implementation. Composes archive material-index lookup with
///        module caching.

#include "Shader/ShaderMap/MaterialShaderMap.h"

#include "Shader/Diagnostics/ShaderLogCategories.h"

#include "Core/Logging/Logger.h"

namespace Hylux::Shader
{

ShaderRef MaterialShaderMap::Get(std::uint64_t passIdHash, std::uint64_t permutationKey,
                                 RHI::ShaderStage stage)
{
    if (archive_ == nullptr || moduleCache_ == nullptr)
    {
        return ShaderRef{};
    }

    const MaterialShaderKey key{materialAssetHash_, passIdHash, permutationKey, stage};
    const auto record = archive_->FindMaterial(key);
    if (!record.has_value())
    {
        HYLUX_LOG_WARN(LogShader,
                       "MaterialShaderMap::Get: miss (material=0x{:016x}, passId=0x{:016x}, "
                       "perm=0x{:016x}, stage={}) in archive {}",
                       materialAssetHash_, passIdHash, permutationKey,
                       static_cast<std::uint32_t>(stage), archive_->DebugName());
        return ShaderRef{};
    }

    auto module = moduleCache_->GetOrCreate(*record);
    if (!module)
    {
        return ShaderRef{};
    }

    ShaderRef ref{};
    ref.module     = module.Get();
    ref.reflection = record->reflection;
    ref.id         = record->id;
    return ref;
}

} // namespace Hylux::Shader
