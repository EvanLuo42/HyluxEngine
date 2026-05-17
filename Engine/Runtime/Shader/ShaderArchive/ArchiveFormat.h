/// @file
/// @brief On-disk layout of a .hslib shader archive. All structs are POD with explicit
///        fixed-size fields and natural alignment so a memory-mapped blob can be cast
///        directly. Little-endian only; the editor compiler is responsible for matching
///        the runtime host byte order.

#pragma once

#include <cstdint>

namespace Hylux::Shader::ArchiveFormat
{

/// @brief Magic bytes prefixing every .hslib file: 'H','S','L','B'.
inline constexpr std::uint32_t kMagic = 0x424C5348u;

/// @brief Current archive format version. Bump on any binary layout change.
inline constexpr std::uint32_t kVersion = 1;

/// @brief Fixed-size header. Always located at byte 0 of the archive.
struct Header
{
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t bytecodeFormat;
    std::uint32_t reservedA;
    std::uint64_t targetTripleHash;
    std::uint32_t passIndexOffset;
    std::uint32_t passIndexCount;
    std::uint32_t materialIndexOffset;
    std::uint32_t materialIndexCount;
    std::uint32_t blobPoolOffset;
    std::uint32_t blobPoolSize;
    std::uint32_t reflectionPoolOffset;
    std::uint32_t reflectionPoolSize;
    std::uint32_t stringPoolOffset;
    std::uint32_t stringPoolSize;
};
static_assert(sizeof(Header) == 64, "ArchiveFormat::Header layout drift");

/// @brief Pass shader index entry. Index is sorted by (passNameHash, permutationKey, stage).
struct PassEntry
{
    std::uint64_t passNameHash;
    std::uint64_t permutationKey;
    std::uint32_t stage;
    std::uint32_t reserved;
    std::uint32_t blobOffset;
    std::uint32_t blobSize;
    std::uint32_t reflectionOffset;
    std::uint32_t reflectionSize;
    std::uint32_t entryPointStringOffset;
    std::uint32_t entryPointStringSize;
    std::uint64_t sourceHash;
};
static_assert(sizeof(PassEntry) == 56, "ArchiveFormat::PassEntry layout drift");

/// @brief Material shader index entry. Index is sorted by (materialAssetHash, passIdHash,
///        permutationKey, stage).
struct MaterialEntry
{
    std::uint64_t materialAssetHash;
    std::uint64_t passIdHash;
    std::uint64_t permutationKey;
    std::uint32_t stage;
    std::uint32_t reserved;
    std::uint32_t blobOffset;
    std::uint32_t blobSize;
    std::uint32_t reflectionOffset;
    std::uint32_t reflectionSize;
    std::uint32_t entryPointStringOffset;
    std::uint32_t entryPointStringSize;
    std::uint64_t sourceHash;
};
static_assert(sizeof(MaterialEntry) == 64, "ArchiveFormat::MaterialEntry layout drift");

} // namespace Hylux::Shader::ArchiveFormat
