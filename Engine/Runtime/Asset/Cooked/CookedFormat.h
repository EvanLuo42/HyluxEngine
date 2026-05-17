/// @file
/// @brief Per-type payload structs for the cooked .hass format. Shared by the runtime
///        loaders (Engine/Runtime/Asset/Loaders/*) and the editor cookers
///        (Engine/Editor/AssetCook/TypeCookers/*) so the on-disk layout is defined in
///        exactly one place.
///
///        All multi-byte fields are little-endian; all offsets are file-relative
///        (measured from byte 0 of the .hass file). All array sections are described by
///        an (offset, count) pair so the loader can reinterpret without further parsing.

#pragma once

#include <cstdint>

namespace Hylux::Asset::Cooked
{

// -----------------------------------------------------------------------------
// Shared sub-payload types
// -----------------------------------------------------------------------------

/// @brief One vertex attribute slot. Semantic / format mirror the engine's input layout
///        enums (encoded as raw u16 so the on-disk format does not depend on enum sizes).
#pragma pack(push, 1)

struct VertexAttribute
{
    std::uint16_t semantic;
    std::uint16_t format;
    std::uint32_t offset;
};

#pragma pack(pop)

static_assert(sizeof(VertexAttribute) == 8);

/// @brief One sub-range of a mesh's index buffer bound to a material slot. Submeshes are
///        non-overlapping; the renderer issues one indirect draw per submesh.
#pragma pack(push, 1)

struct Submesh
{
    std::uint32_t indexStart;
    std::uint32_t indexCount;
    std::uint32_t materialSlot;
};

#pragma pack(pop)

static_assert(sizeof(Submesh) == 12);

/// @brief Per-parameter slot in a MaterialAsset. `kind` is the ParameterKind enum value;
///        `permutationContrib` is bool stored as u8. `uniformOffsetOrTextureSlot` doubles
///        as the uniform-block byte offset for scalar/vector parameters and as the texture
///        bind slot for Texture parameters. `defaultBytes` holds the parameter default in
///        the kind's natural representation (e.g. four floats for Vector, four bytes
///        ignored for Texture — defaults for texture refs go in the RefTable).
#pragma pack(push, 1)

struct ParameterEntry
{
    std::uint64_t nameHash;
    std::uint8_t  kind;
    std::uint8_t  permutationContrib;
    std::uint8_t  pad[2];
    std::uint32_t uniformOffsetOrTextureSlot;
    std::uint8_t  defaultBytes[16];
};

#pragma pack(pop)

static_assert(sizeof(ParameterEntry) == 32);

/// @brief One per-instance override slot. Mirrors ParameterEntry layout closely so the
///        cooker can reuse most of its serialisation logic. `refIndex` points into the
///        owning .hass file's RefTable when `kind` is Texture; otherwise zero.
#pragma pack(push, 1)

struct OverrideEntry
{
    std::uint64_t nameHash;
    std::uint8_t  kind;
    std::uint8_t  pad[3];
    std::uint32_t refIndex;
    std::uint8_t  valueBytes[16];
};

#pragma pack(pop)

static_assert(sizeof(OverrideEntry) == 32);

/// @brief One mip level descriptor. Offsets are relative to TexturePayload::pixelDataOffset.
#pragma pack(push, 1)

struct MipEntry
{
    std::uint32_t offset;
    std::uint32_t size;
};

#pragma pack(pop)

static_assert(sizeof(MipEntry) == 8);

// -----------------------------------------------------------------------------
// Per-type top-level payload structs
// -----------------------------------------------------------------------------

/// @brief MeshAsset payload. typeTag = AssetTypeId::Mesh. Followed in the file by the
///        vertex-layout array, the submesh array, the vertex bytes, and the index bytes;
///        all referenced by file-relative offsets here.
#pragma pack(push, 1)

struct MeshPayload
{
    float         localMin[3];
    float         localMax[3];
    std::uint32_t vertexStride;
    std::uint32_t vertexCount;
    std::uint8_t  indexFormat;
    std::uint8_t  pad0[3];
    std::uint32_t indexCount;
    std::uint32_t vertexLayoutOffset;
    std::uint32_t vertexLayoutCount;
    std::uint32_t submeshOffset;
    std::uint32_t submeshCount;
    std::uint32_t vertexBytesOffset;
    std::uint32_t indexBytesOffset;
};

#pragma pack(pop)

static_assert(sizeof(MeshPayload) == 64);

/// @brief MaterialAsset payload. typeTag = AssetTypeId::Material. Followed in the file
///        by the parameter array referenced by `parameterOffset`.
#pragma pack(push, 1)

struct MaterialPayload
{
    std::uint64_t shaderMapHash;
    std::uint64_t permutationBaseKey;
    std::uint32_t uniformBlockSize;
    std::uint32_t textureCount;
    std::uint32_t parameterOffset;
    std::uint32_t parameterCount;
};

#pragma pack(pop)

static_assert(sizeof(MaterialPayload) == 32);

/// @brief MaterialInstanceAsset payload. typeTag = AssetTypeId::MaterialInstance.
///        `parentRefIndex` selects the parent MaterialAsset in the file's RefTable.
#pragma pack(push, 1)

struct MaterialInstancePayload
{
    std::uint32_t parentRefIndex;
    std::uint32_t overrideOffset;
    std::uint32_t overrideCount;
    std::uint32_t pad;
};

#pragma pack(pop)

static_assert(sizeof(MaterialInstancePayload) == 16);

/// @brief TextureAsset payload. typeTag = AssetTypeId::Texture. Followed in the file by
///        the MipEntry array at `mipTableOffset` and the raw pixel bytes starting at
///        `pixelDataOffset` (mips packed contiguously, indexed via MipEntry offsets).
#pragma pack(push, 1)

struct TexturePayload
{
    std::uint8_t  dimension;
    std::uint8_t  samplerHintFilter;
    std::uint8_t  samplerHintAddress;
    std::uint8_t  pad0;
    std::uint32_t format;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t mipCount;
    std::uint32_t arrayLayers;
    float         samplerHintMaxAniso;
    std::uint32_t mipTableOffset;
    std::uint32_t pixelDataOffset;
};

#pragma pack(pop)

static_assert(sizeof(TexturePayload) == 36);

} // namespace Hylux::Asset::Cooked
