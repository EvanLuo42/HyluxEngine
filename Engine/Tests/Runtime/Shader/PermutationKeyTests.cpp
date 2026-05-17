/// @file
/// @brief Tests for Hylux::Shader permutation-key helpers.

#include "Core/Utils/Hash.h"
#include "Shader/PermutationKey.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Shader");

#include <array>
#include <cstddef>

using namespace Hylux::Shader;

TEST_CASE("Shader::kEmptyPermutationKey == FNV-1a64 offset basis")
{
    static_assert(kEmptyPermutationKey == Hylux::Hash::Fnv1a64Offset);
    PermutationKeyBuilder builder;
    CHECK(builder.Finalize() == kEmptyPermutationKey);
}

TEST_CASE("Shader::Stable*Hash helpers are constexpr and produce stable values")
{
    static_assert(StableTypeId("Hylux.Lighting.PBR") == Hylux::Hash::Fnv1a32("Hylux.Lighting.PBR"));
    static_assert(StablePassNameHash("Hylux.GBuffer") == Hylux::Hash::Fnv1a64("Hylux.GBuffer"));
    static_assert(StableMaterialAssetHash("guid:abc") == Hylux::Hash::Fnv1a64("guid:abc"));
    static_assert(StablePassIdHash("Forward") == Hylux::Hash::Fnv1a64("Forward"));
}

TEST_CASE("PermutationKeyBuilder: AddBool true vs false produce different keys")
{
    PermutationKeyBuilder a, b;
    a.AddBool(true);
    b.AddBool(false);
    CHECK(a.Finalize() != b.Finalize());
    CHECK(a.Finalize() != kEmptyPermutationKey);
}

TEST_CASE("PermutationKeyBuilder: AddInt incorporates the value (different values differ)")
{
    PermutationKeyBuilder a, b;
    a.AddInt(0);
    b.AddInt(1);
    CHECK(a.Finalize() != b.Finalize());

    PermutationKeyBuilder c;
    c.AddInt(0);
    CHECK(c.Finalize() == a.Finalize());
}

TEST_CASE("PermutationKeyBuilder: type tag matters — bool and int with same payload differ")
{
    PermutationKeyBuilder asBool, asInt;
    asBool.AddBool(true);
    asInt.AddInt(1);
    CHECK(asBool.Finalize() != asInt.Finalize());
}

TEST_CASE("PermutationKeyBuilder: AddTypeId mixes the 32-bit id")
{
    PermutationKeyBuilder a, b;
    a.AddTypeId(0xCAFEBABEu);
    b.AddTypeId(0xDEADBEEFu);
    CHECK(a.Finalize() != b.Finalize());
}

TEST_CASE("PermutationKeyBuilder: AddRaw with empty span still mixes the tag + length")
{
    PermutationKeyBuilder empty, nonEmpty;
    empty.AddRaw({});
    const std::array<std::byte, 1> payload{std::byte{0x42}};
    nonEmpty.AddRaw(std::span<const std::byte>(payload.data(), payload.size()));
    CHECK(empty.Finalize() != kEmptyPermutationKey);
    CHECK(empty.Finalize() != nonEmpty.Finalize());
}

TEST_CASE("PermutationKeyBuilder: order of pushes is significant")
{
    PermutationKeyBuilder ab, ba;
    ab.AddBool(true);
    ab.AddInt(1);
    ba.AddInt(1);
    ba.AddBool(true);
    CHECK(ab.Finalize() != ba.Finalize());
}

TEST_CASE("PermutationKeyBuilder: Finalize is idempotent and doesn't mutate state")
{
    PermutationKeyBuilder b;
    b.AddInt(7);
    const auto first = b.Finalize();
    const auto second = b.Finalize();
    CHECK(first == second);
}

TEST_SUITE_END();
