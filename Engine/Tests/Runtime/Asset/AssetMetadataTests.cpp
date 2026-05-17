/// @file
/// @brief Tests for AssetId, AssetRef, AssetTypeId, NameHash.

#include "Asset/AssetId.h"
#include "Asset/AssetRef.h"
#include "Asset/AssetTypeId.h"
#include "Asset/Types/NameHash.h"
#include "Core/Utils/Hash.h"

#include <doctest/doctest.h>

#include <ostream>
#include <string>

using namespace Hylux::Asset;
using Hylux::Guid;

TEST_SUITE_BEGIN("Asset");

// ---- AssetId --------------------------------------------------------------

TEST_CASE("AssetId: default invalid; equality compares guid only (path ignored)")
{
    const AssetId empty;
    CHECK_FALSE(empty.IsValid());

    Guid g{};
    g.bytes[0] = 1;
    const AssetId a{g, "/Engine/foo.hass"};
    const AssetId b{g, "/Game/different.hass"};
    CHECK(a == b);
    CHECK_FALSE(a != b);
    CHECK(a.IsValid());
}

// ---- SoftAssetRef ---------------------------------------------------------

TEST_CASE("SoftAssetRef: default invalid; valid when guid is non-zero")
{
    const SoftAssetRef empty;
    CHECK_FALSE(empty.IsValid());

    Guid g{};
    g.bytes[1] = 9;
    const SoftAssetRef r{g, "/Game/something.hass"};
    CHECK(r.IsValid());
    CHECK(r.pathHint == "/Game/something.hass");
}

// ---- AssetTypeId ----------------------------------------------------------

TEST_CASE("AssetTypeId::ToString: known values + Unknown fallback")
{
    CHECK(ToString(AssetTypeId::Unknown) == "Unknown");
    CHECK(ToString(AssetTypeId::Mesh) == "Mesh");
    CHECK(ToString(AssetTypeId::Material) == "Material");
    CHECK(ToString(AssetTypeId::MaterialInstance) == "MaterialInstance");
    CHECK(ToString(AssetTypeId::Texture) == "Texture");
}

TEST_CASE("AssetTypeId::AssetTypeIdFromString: case-sensitive; unknown returns Unknown")
{
    CHECK(AssetTypeIdFromString("Mesh") == AssetTypeId::Mesh);
    CHECK(AssetTypeIdFromString("Material") == AssetTypeId::Material);
    CHECK(AssetTypeIdFromString("MaterialInstance") == AssetTypeId::MaterialInstance);
    CHECK(AssetTypeIdFromString("Texture") == AssetTypeId::Texture);
    CHECK(AssetTypeIdFromString("mesh") == AssetTypeId::Unknown);
    CHECK(AssetTypeIdFromString("Banana") == AssetTypeId::Unknown);
    CHECK(AssetTypeIdFromString("") == AssetTypeId::Unknown);

    // Round-trip for every known value.
    for (auto id : {AssetTypeId::Mesh, AssetTypeId::Material, AssetTypeId::MaterialInstance, AssetTypeId::Texture})
    {
        CHECK(AssetTypeIdFromString(ToString(id)) == id);
    }
}

// ---- NameHash -------------------------------------------------------------

TEST_CASE("NameHash: MakeNameHash and StableNameHash agree; equal inputs hash equal")
{
    const auto a = MakeNameHash("BaseColor");
    constexpr auto b = StableNameHash("BaseColor");
    CHECK(ToU64(a) == ToU64(b));
    CHECK(a == b);
}

TEST_CASE("NameHash: empty input hashes to FNV-1a64 offset basis (constexpr)")
{
    static_assert(StableNameHash("") == static_cast<NameHash>(Hylux::Hash::Fnv1a64Offset));
    CHECK(ToU64(StableNameHash("")) == Hylux::Hash::Fnv1a64Offset);
}

TEST_CASE("NameHash: Invalid is 0 sentinel; non-empty input rarely produces 0")
{
    static_assert(static_cast<std::uint64_t>(NameHash::Invalid) == 0u);
    CHECK(MakeNameHash("BaseColor") != NameHash::Invalid);
}

TEST_SUITE_END();
