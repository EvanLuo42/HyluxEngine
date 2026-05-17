/// @file
/// @brief Caches RHI shader modules keyed by ShaderId. Translates a ShaderRecord into a
///        Ref<IRHIShaderModule> the first time it is requested, then returns the cached
///        ref on subsequent lookups. Modules survive an archive reload because the RHI
///        backend copies bytecode internally; the cache is cleared explicitly via
///        InvalidateAll when the subsystem reloads.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIShaderModule.h"
#include "RHI/RHIForward.h"
#include "Shader/ShaderArchive/IShaderArchive.h"
#include "Shader/ShaderId.h"

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace Hylux::RHI { class IRHIDevice; }

namespace Hylux::Shader
{

/// @brief Materialization cache for IRHIShaderModule instances. Not thread-safe.
class ShaderModuleCache
{
public:
    ShaderModuleCache(RHI::IRHIDevice* device, const IShaderArchive* archive) noexcept;
    ~ShaderModuleCache() = default;

    ShaderModuleCache(const ShaderModuleCache&)            = delete;
    ShaderModuleCache& operator=(const ShaderModuleCache&) = delete;
    ShaderModuleCache(ShaderModuleCache&&)                 = delete;
    ShaderModuleCache& operator=(ShaderModuleCache&&)      = delete;

    /// @brief Returns the cached module for @p record, or creates one via the RHI device
    ///        if absent. Returns an empty Ref if creation fails (the record is logged).
    Ref<RHI::IRHIShaderModule> GetOrCreate(const ShaderRecord& record);

    /// @brief Drops every cached module reference. Modules with outstanding external refs
    ///        survive until those refs release; new GetOrCreate calls will re-materialize.
    void InvalidateAll() noexcept;

    /// @brief Removes cache entries whose ShaderId no longer exists in @p archive. Used
    ///        after a reload that swapped the archive instance.
    void Prune(const IShaderArchive& archive);

    /// @brief Swaps the archive pointer the cache uses for diagnostics. Does not touch
    ///        existing entries.
    void RebindArchive(const IShaderArchive* archive) noexcept { archive_ = archive; }

    /// @brief Current number of cached modules.
    [[nodiscard]] std::size_t Size() const noexcept { return modules_.size(); }

private:
    RHI::IRHIDevice*      device_;
    const IShaderArchive* archive_;
    std::unordered_map<std::uint64_t, Ref<RHI::IRHIShaderModule>> modules_;
};

} // namespace Hylux::Shader
