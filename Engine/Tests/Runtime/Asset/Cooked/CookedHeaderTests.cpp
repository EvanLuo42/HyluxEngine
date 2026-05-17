/// @file
/// @brief Layout tests for HassHeader / RefEntry POD structs.

#include "Asset/Cooked/CookedFormat.h"
#include "Asset/Cooked/CookedHeader.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Asset::Cooked");

using namespace Hylux::Asset::Cooked;

TEST_CASE("Cooked: HassHeader layout pinned to 128 bytes; RefEntry to 24 bytes")
{
    static_assert(sizeof(HassHeader) == 128);
    static_assert(sizeof(RefEntry) == 24);
    static_assert(kHassHeaderSize == 128);
    CHECK(kHassMagic == 0x53534148u);  // 'HASS' little-endian
    CHECK(kHassEndianTag == 0x01020304u);
    CHECK(kHassVersion == 1);
}

TEST_CASE("Cooked: payload POD struct sizes match cross-tool ABI")
{
    static_assert(sizeof(VertexAttribute) == 8);
    static_assert(sizeof(Submesh) == 12);
    static_assert(sizeof(ParameterEntry) == 32);
    static_assert(sizeof(OverrideEntry) == 32);
    static_assert(sizeof(MipEntry) == 8);
    static_assert(sizeof(MeshPayload) == 64);
    static_assert(sizeof(MaterialPayload) == 32);
    static_assert(sizeof(MaterialInstancePayload) == 16);
    static_assert(sizeof(TexturePayload) == 36);
}

TEST_SUITE_END();
