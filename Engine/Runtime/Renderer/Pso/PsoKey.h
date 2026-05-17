/// @file
/// @brief Four-bucket PSO cache key. Splitting the hash into independent buckets lets the
///        cache invalidate only the entries whose shaderIdsHash changed during a shader hot
///        reload, leaving layout / state / format combinations intact.

#pragma once

#include <cstdint>
#include <functional>

namespace Hylux::Renderer
{

/// @brief Composite cache key. Each bucket is a content hash of one orthogonal axis of the
///        pipeline definition. Buckets are compared independently for equality but the
///        hash function combines all four.
struct PsoKey
{
    std::uint64_t pipelineLayoutHash{0};
    std::uint64_t shaderIdsHash{0};
    std::uint64_t renderStateHash{0};
    std::uint64_t formatAndMultiviewHash{0};

    [[nodiscard]] friend bool operator==(const PsoKey&, const PsoKey&) noexcept = default;
};

/// @brief Hash functor for use with std::unordered_map<PsoKey, ...>.
struct PsoKeyHasher
{
    [[nodiscard]] std::size_t operator()(const PsoKey& key) const noexcept
    {
        // FNV1a-style combination across the four buckets; cheap and well-distributed.
        std::uint64_t h = 1469598103934665603ull;
        const auto    mix = [&](std::uint64_t v) {
            h ^= v;
            h *= 1099511628211ull;
        };
        mix(key.pipelineLayoutHash);
        mix(key.shaderIdsHash);
        mix(key.renderStateHash);
        mix(key.formatAndMultiviewHash);
        return static_cast<std::size_t>(h);
    }
};

} // namespace Hylux::Renderer
