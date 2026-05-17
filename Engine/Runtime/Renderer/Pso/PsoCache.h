/// @file
/// @brief Render-thread PSO resolver. Hides shader resolution, layout cache lookup, and the
///        actual IRHIGraphicsPipeline creation behind a single GetOrCreate call so that
///        render passes never touch the underlying RHI pipeline machinery directly. Owns the
///        backend IRHIPipelineCache and the in-memory key→pipeline table.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIPipelineLayout.h"
#include "RHI/IRHIPipeline.h"
#include "RHI/IRHIPipelineCache.h"
#include "Renderer/Pso/PsoKey.h"
#include "Renderer/Pso/PsoRequest.h"
#include "Shader/ShaderId.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace Hylux::RHI
{
class IRHIDevice;
}

namespace Hylux::Shader
{
class ShaderSubsystem;
}

namespace Hylux::Renderer
{

class PipelineLayoutCache;
class PsoBlobStore;

/// @brief Resolved pipeline + the metadata callers need to bind it. Returned by GetOrCreate.
struct PsoHandle
{
    RHI::IRHIGraphicsPipeline* pipeline{nullptr};
    RHI::IRHIPipelineLayout*   layout{nullptr};
    std::uint32_t              pushConstantSize{0};

    [[nodiscard]] explicit operator bool() const noexcept { return pipeline != nullptr; }
};

/// @brief Per-frame statistics published after each GetOrCreate call. Diagnostic only.
struct PsoCacheStats
{
    std::uint32_t hitCount{0};
    std::uint32_t missCount{0};
    std::uint32_t failureCount{0};
};

/// @brief Resolves PSO requests into runtime IRHIGraphicsPipeline instances. Single-thread:
///        all members assume calls happen on the render thread (stage 1 is also the game
///        thread; stage 3 moves it to a dedicated render thread).
class PsoCache
{
public:
    PsoCache(RHI::IRHIDevice*         device,
             Shader::ShaderSubsystem* shaders,
             PipelineLayoutCache*     layouts,
             RHI::IRHIPipelineCache*  backendCache,
             PsoBlobStore*            blobStore,
             std::string              blobName);

    PsoCache(const PsoCache&)            = delete;
    PsoCache& operator=(const PsoCache&) = delete;

    /// @brief Resolves or compiles the pipeline. Blocks while compiling; the returned
    ///        handle should be cached for the duration of a frame.
    [[nodiscard]] PsoHandle GetOrCreate(const PsoRequest& request);

    /// @brief Drops every cached entry. Stage 2 collapses the per-shader granularity to
    ///        full invalidation; later stages refine to per-shaderIdsHash invalidation.
    void InvalidateAll();

    /// @brief Convenience for callers that already enumerated the affected ids. Stage 2
    ///        ignores the supplied list and falls through to InvalidateAll.
    void InvalidateByShader(std::span<const Shader::ShaderId> ids);

    /// @brief Pulls the backend blob and writes it through the blob store.
    void FlushBlobStore();

    /// @brief Loads the on-disk blob into the backend cache when one exists. Called once
    ///        by RenderSubsystem after constructing the cache.
    void WarmFromBlobStore();

    [[nodiscard]] const PsoCacheStats& GetStats() const noexcept { return stats_; }
    void                               ResetStats() noexcept { stats_ = {}; }

    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }

private:
    RHI::IRHIDevice*          device_{nullptr};
    Shader::ShaderSubsystem*  shaders_{nullptr};
    PipelineLayoutCache*      layouts_{nullptr};
    RHI::IRHIPipelineCache*   backendCache_{nullptr};
    PsoBlobStore*             blobStore_{nullptr};
    std::string               blobName_;

    struct Entry
    {
        Ref<RHI::IRHIGraphicsPipeline> pipeline;
        RHI::IRHIPipelineLayout*       layout{nullptr};
        std::uint32_t                  pushConstantSize{0};
    };

    std::unordered_map<PsoKey, Entry, PsoKeyHasher> entries_;
    std::unordered_set<std::uint64_t>               loggedFailures_;
    PsoCacheStats                                   stats_{};
};

} // namespace Hylux::Renderer
