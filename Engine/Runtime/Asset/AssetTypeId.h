/// @file
/// @brief Discriminator enum for the four v1 asset types. Stored as a u16 typeTag in the
///        cooked .hass header so loaders can dispatch without parsing the payload.

#pragma once

#include <cstdint>
#include <string_view>

namespace Hylux::Asset
{

enum class AssetTypeId : std::uint16_t
{
    Unknown          = 0,
    Mesh             = 1,
    Material         = 2,
    MaterialInstance = 3,
    Texture          = 4,
};

/// @brief Returns the canonical lower-case identifier for an AssetTypeId. Stable across
///        engine versions — the cooker writes the enum value, not this string, but JSON
///        source files use this exact spelling as their "type" field.
[[nodiscard]] constexpr std::string_view ToString(AssetTypeId id) noexcept
{
    switch (id)
    {
        case AssetTypeId::Mesh:             return "Mesh";
        case AssetTypeId::Material:         return "Material";
        case AssetTypeId::MaterialInstance: return "MaterialInstance";
        case AssetTypeId::Texture:          return "Texture";
        case AssetTypeId::Unknown:          break;
    }
    return "Unknown";
}

[[nodiscard]] constexpr AssetTypeId AssetTypeIdFromString(std::string_view name) noexcept
{
    if (name == "Mesh")             return AssetTypeId::Mesh;
    if (name == "Material")         return AssetTypeId::Material;
    if (name == "MaterialInstance") return AssetTypeId::MaterialInstance;
    if (name == "Texture")          return AssetTypeId::Texture;
    return AssetTypeId::Unknown;
}

} // namespace Hylux::Asset
