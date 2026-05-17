/// @file
/// @brief GlobalShaderMap implementation. Composes archive lookup with module caching.

#include "Shader/ShaderMap/GlobalShaderMap.h"

#include "Shader/Diagnostics/ShaderLogCategories.h"

#include "Core/Logging/Logger.h"

namespace Hylux::Shader
{

ShaderRef GlobalShaderMap::Get(std::uint64_t passNameHash, std::uint64_t permutationKey,
                               RHI::ShaderStage stage)
{
    if (archive_ == nullptr || moduleCache_ == nullptr)
    {
        return ShaderRef{};
    }

    const PassShaderKey key{passNameHash, permutationKey, stage};
    const auto record = archive_->FindPass(key);
    if (!record.has_value())
    {
        HYLUX_LOG_WARN(LogShader,
                       "GlobalShaderMap::Get: miss (passNameHash=0x{:016x}, perm=0x{:016x}, "
                       "stage={}) in archive {}",
                       passNameHash, permutationKey, static_cast<std::uint32_t>(stage),
                       archive_->DebugName());
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

void GlobalShaderMap::LogContents() const
{
    if (archive_ == nullptr)
    {
        return;
    }
    HYLUX_LOG_INFO(LogShader, "GlobalShaderMap: archive {} carries {} pass entries",
                   archive_->DebugName(), archive_->PassEntryCount());
}

} // namespace Hylux::Shader
