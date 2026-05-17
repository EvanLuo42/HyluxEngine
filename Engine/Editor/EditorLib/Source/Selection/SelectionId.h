/// @file
/// @brief Strongly-typed selection identifier. A SelectionId is the (kind, handle) pair
///        that uniquely names a selectable across the editor — kind discriminates the
///        adapter (entity, asset, material…) and handle is a kind-specific opaque 64-bit
///        value (entity index, asset guid hash, etc.).

#pragma once

#include "Core/Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace Hylux::Editor
{

/// @brief Value type. Comparable and hashable so it can key std containers / Qt models.
struct SelectionId
{
    TypeId        kind{TypeId::Invalid};
    std::uint64_t handle{0};

    [[nodiscard]] friend bool operator==(SelectionId a, SelectionId b) noexcept
    {
        return a.kind == b.kind && a.handle == b.handle;
    }
    [[nodiscard]] friend bool operator!=(SelectionId a, SelectionId b) noexcept { return !(a == b); }
};

} // namespace Hylux::Editor

template<> struct std::hash<Hylux::Editor::SelectionId>
{
    [[nodiscard]] std::size_t operator()(Hylux::Editor::SelectionId id) const noexcept
    {
        const std::uint64_t k = Hylux::ToU64(id.kind);
        const std::uint64_t mixed = k * 0x9E3779B97F4A7C15ULL + id.handle;
        return static_cast<std::size_t>(mixed ^ (mixed >> 32));
    }
};
