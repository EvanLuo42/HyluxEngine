/// @file
/// @brief Per-material view onto the material-shader subset of a ShaderArchive. Holds the
///        material's stable asset hash so callers only supply the pass id, permutation,
///        and stage on lookup.

#pragma once

#include "Shader/ShaderArchive/IShaderArchive.h"
#include "Shader/ShaderMap/ShaderModuleCache.h"
#include "Shader/ShaderRef.h"

namespace Hylux::Shader
{

/// @brief Lookup view for a single material asset. Cheap to construct; the
///        MaterialShaderMapCache holds these by materialAssetHash and rebuilds them on
///        archive reload.
class MaterialShaderMap
{
public:
    MaterialShaderMap(std::uint64_t materialAssetHash, const IShaderArchive* archive,
                      ShaderModuleCache* moduleCache) noexcept
        : materialAssetHash_(materialAssetHash), archive_(archive), moduleCache_(moduleCache)
    {
    }

    /// @brief Looks up the material shader for a given pass usage and materializes the
    ///        RHI module on demand.
    [[nodiscard]] ShaderRef Get(std::uint64_t passIdHash, std::uint64_t permutationKey,
                                RHI::ShaderStage stage);

    [[nodiscard]] std::uint64_t MaterialAssetHash() const noexcept { return materialAssetHash_; }

private:
    std::uint64_t         materialAssetHash_;
    const IShaderArchive* archive_;
    ShaderModuleCache*    moduleCache_;
};

} // namespace Hylux::Shader
