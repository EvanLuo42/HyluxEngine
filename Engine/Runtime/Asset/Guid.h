/// @file
/// @brief 128-bit GUID type. Asset identity is path-plus-GUID; the path is the human
///        face of an asset, the GUID is the rename-safe stable id baked into the file
///        at creation time. Byte layout matches the cooked .hass header so the bytes
///        can be memcpy'd in/out without endian or packing fixups.

#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace Hylux::Asset
{

/// @brief 128-bit GUID stored as 16 bytes in network/big-endian-ish layout — actually
///        the canonical RFC-4122 byte order, where ToString() formats bytes 0..15 as
///        "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx". Comparison is byte-lexicographic;
///        Hash() is a 64-bit fold suitable for unordered_map.
struct Guid
{
    static constexpr std::size_t kSize = 16;

    std::array<std::uint8_t, kSize> bytes{};

    [[nodiscard]] static constexpr Guid Zero() noexcept { return Guid{}; }

    /// @brief Returns a freshly generated RFC-4122 v4 (random) GUID. Editor / cooker
    ///        use this at asset creation time; runtime should rarely need it.
    [[nodiscard]] static Guid Generate();

    /// @brief Parses canonical 8-4-4-4-12 hex form. Hyphens are required; case is ignored.
    ///        Returns nullopt on any malformed input.
    [[nodiscard]] static std::optional<Guid> Parse(std::string_view text);

    /// @brief Formats as canonical lower-case "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] bool IsZero() const noexcept;

    /// @brief 64-bit hash for use in unordered_map. Folds the two 64-bit halves with xor;
    ///        v4 GUIDs are random enough that this is collision-free in practice.
    [[nodiscard]] std::uint64_t Hash() const noexcept;

    [[nodiscard]] friend constexpr bool operator==(const Guid& a, const Guid& b) noexcept
    {
        return a.bytes == b.bytes;
    }

    [[nodiscard]] friend constexpr auto operator<=>(const Guid& a, const Guid& b) noexcept
    {
        return a.bytes <=> b.bytes;
    }
};

} // namespace Hylux::Asset

template<>
struct std::hash<Hylux::Asset::Guid>
{
    std::size_t operator()(const Hylux::Asset::Guid& g) const noexcept
    {
        return static_cast<std::size_t>(g.Hash());
    }
};
