/// @file
/// @brief Parameter descriptor + tagged value used by MaterialAsset / MaterialInstance.

#pragma once

#include "Asset/Types/NameHash.h"

#include <cstdint>

namespace Hylux::Asset
{

/// @brief Logical type of a material parameter slot.
enum class ParameterKind : std::uint8_t
{
    Scalar,
    Vector,
    Texture,
};

/// @brief Tagged value union. Plain struct (no std::variant) so a MaterialSnapshot can
///        hold a flat vector of these and ship them across the structural command queue.
///        For Texture parameters `textureBindlessIndex` is the slot reported by
///        TextureAsset::GetBindlessIndex(); 0xFFFFFFFF means "no texture bound".
struct ParameterValue
{
    static constexpr std::uint32_t kInvalidBindlessIndex = 0xFFFFFFFFu;

    ParameterKind kind{ParameterKind::Scalar};
    float         vec[4]{};
    std::uint32_t textureBindlessIndex{kInvalidBindlessIndex};
};

/// @brief Schema entry baked into a MaterialAsset. Describes where the parameter lives
///        in the uniform block (for scalar / vector) or in the texture-handle slot array
///        (for texture).
struct ParameterDesc
{
    NameHash      name{NameHash::Invalid};
    ParameterKind kind{ParameterKind::Scalar};

    /// @brief True if a change to this parameter changes the shader permutation key.
    bool isPermutation{false};

    /// @brief Byte offset of the value inside the per-instance uniform block (scalar /
    ///        vector only).
    std::uint32_t uniformOffset{0};

    /// @brief Index into MaterialSnapshot::textureBindlessIndices / MaterialProxy texture
    ///        bindless array (texture only).
    std::uint32_t textureSlot{0};
};

} // namespace Hylux::Asset
