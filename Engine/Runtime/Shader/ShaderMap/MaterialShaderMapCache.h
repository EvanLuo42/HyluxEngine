/// @file
/// @brief Caches MaterialShaderMap instances keyed by materialAssetHash so each material
///        has a single view onto the archive's material-shader subset.

#pragma once

#include "Shader/ShaderMap/MaterialShaderMap.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Hylux::Shader
{

class IShaderArchive;
class ShaderModuleCache;

/// @brief Per-subsystem owning store of MaterialShaderMap instances.
class MaterialShaderMapCache
{
public:
    MaterialShaderMapCache(const IShaderArchive* archive,
                           ShaderModuleCache* moduleCache) noexcept
        : archive_(archive), moduleCache_(moduleCache)
    {
    }

    /// @brief Returns the cached MaterialShaderMap for @p materialAssetHash, constructing
    ///        one if absent.
    MaterialShaderMap* GetOrLoad(std::uint64_t materialAssetHash);

    /// @brief Clears every cached MaterialShaderMap. Called on archive reload.
    void Clear() noexcept { maps_.clear(); }

    /// @brief Swaps the archive + cache pointers used to construct new maps.
    void Rebind(const IShaderArchive* archive, ShaderModuleCache* moduleCache) noexcept
    {
        archive_     = archive;
        moduleCache_ = moduleCache;
    }

private:
    const IShaderArchive*  archive_;
    ShaderModuleCache*     moduleCache_;
    std::unordered_map<std::uint64_t, std::unique_ptr<MaterialShaderMap>> maps_;
};

} // namespace Hylux::Shader
