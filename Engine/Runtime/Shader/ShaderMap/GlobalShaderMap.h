/// @file
/// @brief View onto the pass-shader subset of a ShaderArchive. Translates a
///        (passNameHash, permutationKey, stage) triple into a ShaderRef by going through
///        the archive's index and the module cache.

#pragma once

#include "Shader/ShaderArchive/IShaderArchive.h"
#include "Shader/ShaderMap/ShaderModuleCache.h"
#include "Shader/ShaderRef.h"

namespace Hylux::Shader
{

/// @brief Pass-shader lookup view. Stateless beyond its two non-owning pointers; the
///        owning ShaderSubsystem swaps these on reload.
class GlobalShaderMap
{
public:
    GlobalShaderMap(const IShaderArchive* archive, ShaderModuleCache* moduleCache) noexcept
        : archive_(archive), moduleCache_(moduleCache)
    {
    }

    /// @brief Looks up the pass shader and materializes its RHI module on demand.
    [[nodiscard]] ShaderRef Get(std::uint64_t passNameHash, std::uint64_t permutationKey,
                                RHI::ShaderStage stage);

    /// @brief Re-points the view at a freshly opened archive and cache (used after reload).
    void Rebind(const IShaderArchive* archive, ShaderModuleCache* moduleCache) noexcept
    {
        archive_     = archive;
        moduleCache_ = moduleCache;
    }

    /// @brief Logs every pass entry currently visible. Diagnostic helper.
    void LogContents() const;

private:
    const IShaderArchive* archive_;
    ShaderModuleCache*    moduleCache_;
};

} // namespace Hylux::Shader
