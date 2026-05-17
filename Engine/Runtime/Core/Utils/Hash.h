/// @file
/// @brief constexpr FNV-1a hashing utilities shared across the Hylux runtime.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

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

/// @brief Mixes one byte into an FNV-1a 64-bit accumulator.
[[nodiscard]] constexpr std::uint64_t MixU8(std::uint64_t seed, std::uint8_t value) noexcept
{
    return (seed ^ static_cast<std::uint64_t>(value)) * Fnv1a64Prime;
}

/// @brief Mixes a 32-bit value into an FNV-1a 64-bit accumulator as four little-endian
///        bytes. Byte order is fixed so hashes match across platforms and tools that
///        share a key (e.g. editor vs runtime shader id).
[[nodiscard]] constexpr std::uint64_t MixU32(std::uint64_t seed, std::uint32_t value) noexcept
{
    seed = MixU8(seed, static_cast<std::uint8_t>(value & 0xFFu));
    seed = MixU8(seed, static_cast<std::uint8_t>((value >>  8) & 0xFFu));
    seed = MixU8(seed, static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    seed = MixU8(seed, static_cast<std::uint8_t>((value >> 24) & 0xFFu));
    return seed;
}

/// @brief Mixes a 64-bit value into an FNV-1a 64-bit accumulator as eight little-endian
///        bytes. See MixU32 for portability notes.
[[nodiscard]] constexpr std::uint64_t MixU64(std::uint64_t seed, std::uint64_t value) noexcept
{
    seed = MixU32(seed, static_cast<std::uint32_t>(value & 0xFFFFFFFFu));
    seed = MixU32(seed, static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFFu));
    return seed;
}

/// @brief Mixes a raw byte range into an existing FNV-1a 64-bit accumulator. Equivalent
///        to Fnv1a64(data, length, seed) but spells the seeded-continue intent at the
///        call site. Use for runtime data of opaque layout (struct content hashing); use
///        MixU* when portability across platforms matters.
[[nodiscard]] inline std::uint64_t MixBytes(std::uint64_t seed, const void* data, std::size_t length) noexcept
{
    return Fnv1a64(static_cast<const char*>(data), length, seed);
}

/// @brief Mixes the raw bytes of a trivially-copyable value into an accumulator. Reads
///        sizeof(T) bytes through std::memcpy so it is well-defined for any trivially
///        copyable T, including ones whose pointer-to-byte cast would otherwise be a
///        strict-aliasing violation.
template<typename T>
[[nodiscard]] inline std::uint64_t MixTrivial(std::uint64_t seed, const T& value) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "Hash::MixTrivial<T>: T must be trivially copyable");
    char bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
    return Fnv1a64(bytes, sizeof(T), seed);
}

/// @brief Combines an already-hashed 64-bit value into an accumulator with a single
///        xor-and-prime step. Cheaper than MixU64 because it skips the per-byte loop;
///        appropriate when `value` is itself a content hash, so its bits are already
///        well-distributed. Not bit-equivalent to MixU64 — pick one per call site and
///        stick with it across editor / runtime to keep keys stable.
[[nodiscard]] constexpr std::uint64_t Combine(std::uint64_t seed, std::uint64_t value) noexcept
{
    return (seed ^ value) * Fnv1a64Prime;
}

} // namespace Hylux::Hash
