/// @file
/// @brief Four-bucket PSO cache key. Splitting the hash into independent buckets lets the
///        cache invalidate only the entries whose shaderIdsHash changed during a shader hot
///        reload, leaving layout / state / format combinations intact.

#pragma once

#include "Core/Utils/Hash.h"

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
        std::uint64_t h = Hash::Fnv1a64Offset;
        h = Hash::Combine(h, key.pipelineLayoutHash);
        h = Hash::Combine(h, key.shaderIdsHash);
        h = Hash::Combine(h, key.renderStateHash);
        h = Hash::Combine(h, key.formatAndMultiviewHash);
        return static_cast<std::size_t>(h);
    }
};

} // namespace Hylux::Renderer
