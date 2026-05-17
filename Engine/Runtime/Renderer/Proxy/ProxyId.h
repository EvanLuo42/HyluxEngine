/// @file
/// @brief Strongly-typed identifier handed out by RenderSubsystem when a primitive proxy is
///        spawned. Stable across structural mutations; indexes the TransformDoubleBuffer
///        and ProxyRegistry slots once GPU-driven culling lands in a later stage.

#pragma once

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Render-side primitive identifier. The sentinel Invalid signifies "no proxy".
enum class ProxyId : std::uint32_t
{
    Invalid = 0xFFFFFFFFu,
};

[[nodiscard]] constexpr bool IsValid(const ProxyId id) noexcept
{
    return id != ProxyId::Invalid;
}

[[nodiscard]] constexpr std::uint32_t ToU32(ProxyId id) noexcept
{
    return static_cast<std::uint32_t>(id);
}

} // namespace Hylux::Renderer
