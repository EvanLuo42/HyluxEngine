/// @file
/// @brief Permutation key encoding used to address shader variants in a ShaderArchive.
///        Mirrors the Slang specialization argument tuple as a sequence of typed pushes
///        into a builder; the final 64-bit hash is the runtime lookup key. The editor's
///        shader compiler and the renderer's call sites must push the same arguments in
///        the same order to obtain matching keys.

#pragma once

#include "Core/Utils/Hash.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace Hylux::Shader
{

/// @brief Sentinel permutation key meaning "no specialization arguments". Used by shaders
///        whose schema is empty (the only variant is the default one).
inline constexpr std::uint64_t kEmptyPermutationKey = Hash::Fnv1a64Offset;

/// @brief Computes a stable 32-bit identifier for a Slang generic argument type (e.g. the
///        concrete `ILighting` implementation passed to a `Material<TLighting>`). Editor
///        and runtime independently call this with the same fully-qualified type name to
///        obtain the same id.
[[nodiscard]] constexpr std::uint32_t StableTypeId(std::string_view fullyQualifiedName) noexcept
{
    return Hash::Fnv1a32(fullyQualifiedName);
}

/// @brief Computes a stable 64-bit identifier for a pass shader's qualified name (e.g.
///        "Hylux.GBufferPass"). Editor records the same hash in the archive index.
[[nodiscard]] constexpr std::uint64_t StablePassNameHash(std::string_view passName) noexcept
{
    return Hash::Fnv1a64(passName);
}

/// @brief Computes a stable 64-bit identifier for a material asset (by content GUID or
///        stable path) using the same algorithm shared with the editor side.
[[nodiscard]] constexpr std::uint64_t StableMaterialAssetHash(std::string_view assetKey) noexcept
{
    return Hash::Fnv1a64(assetKey);
}

/// @brief Computes a stable 64-bit identifier for a pass usage id (e.g. "Forward",
///        "GBuffer", "Shadow") consumed by the material shader index.
[[nodiscard]] constexpr std::uint64_t StablePassIdHash(std::string_view passId) noexcept
{
    return Hash::Fnv1a64(passId);
}

/// @brief Accumulates specialization arguments into a 64-bit permutation key. The order
///        of pushes is significant: it must match the schema baked into the archive by
///        the editor compiler, or the resulting key will not address any entry.
class PermutationKeyBuilder
{
public:
    PermutationKeyBuilder() noexcept = default;

    /// @brief Pushes a boolean argument (Slang `extern static const bool` slot).
    void AddBool(bool value) noexcept;

    /// @brief Pushes a signed integer argument (Slang `extern static const int`-shaped slot).
    ///        Encoded as little-endian 8 bytes regardless of the original width.
    void AddInt(std::int64_t value) noexcept;

    /// @brief Pushes a generic type argument by its StableTypeId.
    void AddTypeId(std::uint32_t stableTypeId) noexcept;

    /// @brief Pushes a raw byte span; use for `extern static const` slots whose value is
    ///        a struct or array. The byte order shipped by the editor must match.
    void AddRaw(std::span<const std::byte> bytes) noexcept;

    /// @brief Returns the accumulated 64-bit hash. May be called multiple times.
    [[nodiscard]] std::uint64_t Finalize() const noexcept { return hash_; }

private:
    std::uint64_t hash_{Hash::Fnv1a64Offset};
};

} // namespace Hylux::Shader
