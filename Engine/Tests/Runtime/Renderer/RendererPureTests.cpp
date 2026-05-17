/// @file
/// @brief Pure-CPU tests for Renderer types: ProxyId, PsoKey, MaterialInstance / Snapshot.

#include "Asset/AssetId.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/MaterialParameter.h"
#include "Asset/Types/NameHash.h"
#include "Core/Memory/MakeRef.h"
#include "Renderer/Material/MaterialInstance.h"
#include "Renderer/Material/MaterialSnapshot.h"
#include "Renderer/Proxy/ProxyId.h"
#include "Renderer/Pso/PsoKey.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Renderer");

#include <cstring>
#include <unordered_map>
#include <vector>

using namespace Hylux;
using namespace Hylux::Renderer;

// ---- ProxyId --------------------------------------------------------------

TEST_CASE("ProxyId::Invalid sentinel + IsValid + ToU32")
{
    static_assert(static_cast<std::uint32_t>(ProxyId::Invalid) == 0xFFFFFFFFu);
    static_assert(!IsValid(ProxyId::Invalid));
    static_assert(IsValid(static_cast<ProxyId>(0)));
    static_assert(IsValid(static_cast<ProxyId>(0xFFFFFFFEu)));
    static_assert(ToU32(static_cast<ProxyId>(7)) == 7u);
}

// ---- PsoKey --------------------------------------------------------------

TEST_CASE("PsoKey: equality compares all four buckets")
{
    PsoKey a{1, 2, 3, 4};
    PsoKey b{1, 2, 3, 4};
    PsoKey c{1, 2, 3, 5};
    CHECK(a == b);
    CHECK_FALSE(a == c);
}

TEST_CASE("PsoKey: PsoKeyHasher is deterministic and bucket-dependent")
{
    PsoKey a{1, 2, 3, 4};
    PsoKey b{1, 2, 3, 4};
    PsoKey c{1, 2, 3, 5};
    PsoKeyHasher h;
    CHECK(h(a) == h(b));
    CHECK(h(a) != h(c));
}

TEST_CASE("PsoKey: usable as unordered_map key")
{
    std::unordered_map<PsoKey, int, PsoKeyHasher> m;
    m[PsoKey{1, 2, 3, 4}] = 100;
    m[PsoKey{1, 2, 3, 5}] = 200;
    CHECK(m.size() == 2u);
    CHECK(m[PsoKey{1, 2, 3, 4}] == 100);
}

// ---- MaterialInstance ----------------------------------------------------

namespace
{

Ref<Asset::MaterialAsset> MakeMaterialAsset()
{
    Asset::MaterialAsset::InitData data{};
    data.shaderMapHash      = 0xABCDull;
    data.permutationBaseKey = 0x1234ull;
    data.uniformBlockSize   = 64;
    data.textureCount       = 2;

    Asset::ParameterDesc scalar{};
    scalar.name          = Asset::MakeNameHash("Roughness");
    scalar.kind          = Asset::ParameterKind::Scalar;
    scalar.isPermutation = false;
    scalar.uniformOffset = 0;
    Asset::ParameterDesc vector{};
    vector.name          = Asset::MakeNameHash("BaseColor");
    vector.kind          = Asset::ParameterKind::Vector;
    vector.uniformOffset = 16;
    Asset::ParameterDesc texSlot0{};
    texSlot0.name          = Asset::MakeNameHash("AlbedoTex");
    texSlot0.kind          = Asset::ParameterKind::Texture;
    texSlot0.textureSlot   = 0;
    Asset::ParameterDesc texSlot1{};
    texSlot1.name          = Asset::MakeNameHash("NormalTex");
    texSlot1.kind          = Asset::ParameterKind::Texture;
    texSlot1.textureSlot   = 1;
    texSlot1.isPermutation = true;  // toggles a permutation if overridden

    data.parameters = {scalar, vector, texSlot0, texSlot1};

    return MakeRef<Asset::MaterialAsset>(Asset::AssetId{}, std::move(data));
}

} // namespace

TEST_CASE("MaterialInstance: SetScalar / SetVector update overrides; unknown / wrong-kind ignored")
{
    auto asset = MakeMaterialAsset();
    MaterialInstance inst{asset.Get()};
    CHECK(inst.GetAsset() == asset.Get());
    CHECK(inst.GetAssetHash() == asset->GetShaderMapHash());

    const auto rough = Asset::MakeNameHash("Roughness");
    inst.SetScalar(rough, 0.5f);

    // Wrong kind: ignored.
    inst.SetVector(rough, 1, 0, 0, 1);

    // Unknown name: ignored.
    inst.SetScalar(Asset::MakeNameHash("NotAParameter"), 99.0f);

    auto snap = inst.Snapshot();
    REQUIRE(snap.uniformBlock.size() == 64u);
    float roughBytes{};
    std::memcpy(&roughBytes, snap.uniformBlock.data() + 0, sizeof(float));
    CHECK(roughBytes == 0.5f);
}

TEST_CASE("MaterialInstance: GetInstanceHash / GetPermutationKey are cached and dirtied on writes")
{
    auto asset = MakeMaterialAsset();
    MaterialInstance inst{asset.Get()};
    const auto hash0 = inst.GetInstanceHash();
    const auto perm0 = inst.GetPermutationKey();
    CHECK(inst.GetInstanceHash() == hash0);
    CHECK(inst.GetPermutationKey() == perm0);

    inst.SetVector(Asset::MakeNameHash("BaseColor"), 0.1f, 0.2f, 0.3f, 1.0f);
    CHECK(inst.GetInstanceHash() != hash0);
}

TEST_CASE("MaterialInstance: SetTexture(null) clears the override and is a no-op on unknown name")
{
    auto asset = MakeMaterialAsset();
    MaterialInstance inst{asset.Get()};
    inst.SetTexture(Asset::MakeNameHash("AlbedoTex"), Ref<Asset::TextureAsset>{});
    inst.SetTexture(Asset::MakeNameHash("NoSuchTex"), Ref<Asset::TextureAsset>{});

    auto snap = inst.Snapshot();
    CHECK(snap.textureBindlessIndices.size() == 2u);
}

TEST_CASE("MaterialInstance::Snapshot: writes vector to its uniform offset")
{
    auto asset = MakeMaterialAsset();
    MaterialInstance inst{asset.Get()};
    inst.SetVector(Asset::MakeNameHash("BaseColor"), 1.0f, 0.5f, 0.25f, 0.0f);
    auto snap = inst.Snapshot();
    REQUIRE(snap.uniformBlock.size() == 64u);
    float vec[4] = {};
    std::memcpy(vec, snap.uniformBlock.data() + 16, sizeof(vec));
    CHECK(vec[0] == 1.0f);
    CHECK(vec[1] == 0.5f);
    CHECK(vec[2] == 0.25f);
    CHECK(vec[3] == 0.0f);
}

TEST_CASE("MaterialInstance: null asset yields an empty snapshot with zeroed metadata")
{
    MaterialInstance inst{nullptr};
    CHECK(inst.GetAsset() == nullptr);
    CHECK(inst.GetAssetHash() == 0u);
    auto snap = inst.Snapshot();
    CHECK(snap.materialAssetHash == 0u);
    CHECK(snap.uniformBlock.empty());
    CHECK(snap.textureBindlessIndices.empty());
}

TEST_CASE("MaterialAsset::FindParameter: hit and miss")
{
    auto asset = MakeMaterialAsset();
    CHECK(asset->FindParameter(Asset::MakeNameHash("BaseColor")) != nullptr);
    CHECK(asset->FindParameter(Asset::MakeNameHash("Unknown")) == nullptr);
    CHECK(asset->GetShaderMapHash() == 0xABCDull);
    CHECK(asset->GetPermutationBaseKey() == 0x1234ull);
    CHECK(asset->GetUniformBlockSize() == 64u);
    CHECK(asset->GetTextureCount() == 2u);
    CHECK(asset->GetParameters().size() == 4u);
    CHECK(asset->GetShaderMap() == nullptr);
    CHECK(asset->GetMemoryFootprint() == 0u);
}

TEST_SUITE_END();
