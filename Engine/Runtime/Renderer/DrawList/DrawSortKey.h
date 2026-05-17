/// @file
/// @brief Sort-key utilities used by DrawListBuilder. A 64-bit packed key is composed of
///        (depth bits, material id bits, primitive id bits) so std::sort can order draws
///        front-to-back, back-to-front, or material-then-depth depending on bit layout.

#pragma once

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Opaque 64-bit key. Lower bits encode finer-grained tie-breakers (primitive id),
///        upper bits encode the primary sort axis (depth or material).
using DrawSortKey = std::uint64_t;

/// @brief Computes a front-to-back sort key. depth32 is the IEEE-754 bit pattern of a
///        non-negative float; primitiveId is the secondary tie-breaker.
[[nodiscard]] constexpr DrawSortKey MakeDepthFrontToBackKey(std::uint32_t depth32,
                                                            std::uint32_t primitiveId) noexcept
{
    return (static_cast<std::uint64_t>(depth32) << 32) | static_cast<std::uint64_t>(primitiveId);
}

/// @brief Computes a back-to-front sort key by inverting the depth bits before composition.
[[nodiscard]] constexpr DrawSortKey MakeDepthBackToFrontKey(std::uint32_t depth32,
                                                            std::uint32_t primitiveId) noexcept
{
    return (static_cast<std::uint64_t>(~depth32) << 32) | static_cast<std::uint64_t>(primitiveId);
}

/// @brief Material-then-depth ordering: material id occupies the high 32 bits, depth the
///        low 32 bits to break ties within a material.
[[nodiscard]] constexpr DrawSortKey MakeMaterialThenDepthKey(std::uint32_t materialId,
                                                             std::uint32_t depth32) noexcept
{
    return (static_cast<std::uint64_t>(materialId) << 32) | static_cast<std::uint64_t>(depth32);
}

} // namespace Hylux::Renderer
