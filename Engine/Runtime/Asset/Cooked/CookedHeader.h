/// @file
/// @brief Fixed 128-byte header at offset 0 of every cooked .hass file. Layout is
///        intentionally over-aligned and padded so reinterpret_cast straight off the
///        memory buffer is safe on every platform the engine targets (x64, ARM64).

#pragma once

#include "Asset/AssetTypeId.h"

#include <cstdint>

namespace Hylux::Asset::Cooked
{

/// @brief Magic spelled 'HASS' little-endian (= 0x53534148). Distinguishes cooked Hylux
///        asset files from anything else; loaders refuse to parse on mismatch.
inline constexpr std::uint32_t kHassMagic = 0x53534148u;

/// @brief Sentinel detecting endian mismatch. Cooker writes the constant 0x01020304;
///        a reader that observes 0x04030201 has loaded a big-endian file and bails out.
inline constexpr std::uint32_t kHassEndianTag = 0x01020304u;

inline constexpr std::uint16_t kHassVersion = 1;

/// @brief Total bytes the header occupies in the cooked file. The reserved tail is
///        large enough to absorb a few future fields without breaking the on-disk
///        offsets of payload / reftable sections that come after.
inline constexpr std::uint32_t kHassHeaderSize = 128;

#pragma pack(push, 1)

/// @brief On-disk header. Multi-byte fields are little-endian; the loader validates
///        kHassEndianTag to detect a big-endian file (currently unsupported).
struct HassHeader
{
    std::uint32_t magic;
    std::uint32_t endianTag;
    std::uint16_t version;
    std::uint16_t typeTag;
    std::uint8_t  guid[16];
    std::uint32_t payloadOffset;
    std::uint32_t payloadSize;
    std::uint32_t refTableOffset;
    std::uint32_t refTableCount;
    std::uint8_t  contentHash[32];
    std::uint8_t  reserved[52];
};

#pragma pack(pop)

static_assert(sizeof(HassHeader) == kHassHeaderSize, "HassHeader must be exactly 128 bytes");

/// @brief Fixed-size descriptor for one cross-asset reference. The path text itself
///        lives in a string pool immediately after the ref-entry array; pathOffset is
///        relative to that pool. Total descriptor size is 24 bytes (naturally aligned).
#pragma pack(push, 1)

struct RefEntry
{
    std::uint8_t  guid[16];
    std::uint32_t pathOffset;
    std::uint32_t pathLength;
};

#pragma pack(pop)

static_assert(sizeof(RefEntry) == 24, "RefEntry must be exactly 24 bytes");

} // namespace Hylux::Asset::Cooked
