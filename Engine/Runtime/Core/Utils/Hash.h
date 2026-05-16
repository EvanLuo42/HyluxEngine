/// @file
/// @brief constexpr FNV-1a hashing utilities shared across the Hylux runtime.

#pragma once

#include <cstdint>
#include <string_view>

namespace Hylux::Hash
{

inline constexpr std::uint64_t Fnv1a64Offset = 0xcbf29ce484222325ULL;
inline constexpr std::uint64_t Fnv1a64Prime = 0x00000100000001b3ULL;
inline constexpr std::uint32_t Fnv1a32Offset = 0x811c9dc5u;
inline constexpr std::uint32_t Fnv1a32Prime = 0x01000193u;

/// @brief Computes FNV-1a 64-bit hash over a raw byte range.
[[nodiscard]] constexpr std::uint64_t Fnv1a64(const char* data, const std::size_t length,
                                              const std::uint64_t seed = Fnv1a64Offset) noexcept
{
    std::uint64_t hash = seed;
    for (std::size_t i = 0; i < length; ++i)
    {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(data[i]));
        hash *= Fnv1a64Prime;
    }
    return hash;
}

/// @brief Computes FNV-1a 64-bit hash of a string view.
[[nodiscard]] constexpr std::uint64_t Fnv1a64(std::string_view text, const std::uint64_t seed = Fnv1a64Offset) noexcept
{
    return Fnv1a64(text.data(), text.size(), seed);
}

/// @brief Computes FNV-1a 32-bit hash of a string view.
[[nodiscard]] constexpr std::uint32_t Fnv1a32(std::string_view text, const std::uint32_t seed = Fnv1a32Offset) noexcept
{
    std::uint32_t hash = seed;
    for (const char c : text)
    {
        hash ^= static_cast<std::uint32_t>(static_cast<unsigned char>(c));
        hash *= Fnv1a32Prime;
    }
    return hash;
}

} // namespace Hylux::Hash
