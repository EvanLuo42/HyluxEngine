/// @file
/// @brief Caches IRHIPipelineLayout instances keyed by ShaderReflection::pipelineLayoutHash.
///        Render passes never construct layouts directly; the PsoCache resolves them through
///        this cache when assembling a PSO request.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIPipelineLayout.h"
#include "Shader/ShaderReflection.h"

#include <cstdint>
#include <unordered_map>

namespace Hylux::RHI
{
class IRHIDevice;
}

namespace Hylux::Renderer
{

/// @brief Owns the active map of `pipelineLayoutHash -> IRHIPipelineLayout`. Construction
///        walks ShaderReflection to populate the PipelineLayoutDesc — currently push
///        constants plus the bindless flags; explicit descriptor sets land later.
class PipelineLayoutCache
{
public:
    explicit PipelineLayoutCache(RHI::IRHIDevice* device) noexcept;

    PipelineLayoutCache(const PipelineLayoutCache&)            = delete;
    PipelineLayoutCache& operator=(const PipelineLayoutCache&) = delete;

    /// @brief Returns the cached layout for the supplied reflection, lazily constructing
    ///        one on miss. Returns nullptr if construction fails.
    [[nodiscard]] RHI::IRHIPipelineLayout* GetOrCreate(std::uint64_t                  layoutHash,
                                                       const Shader::ShaderReflection& reflection);

    /// @brief Drops every cached layout. Called from RenderSubsystem::Shutdown.
    void Clear() noexcept;

    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }

private:
    RHI::IRHIDevice* device_{nullptr};

    std::unordered_map<std::uint64_t, Ref<RHI::IRHIPipelineLayout>> entries_;
};

} // namespace Hylux::Renderer
