/// @file
/// @brief Tests for Vec3 (SIMD-padded) and Vec4 (full SIMD lane) in Core/Math.

#include "Core/Math/Common.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Math");

#include <cmath>

using namespace Hylux::Math;

namespace
{

constexpr float kTol = 1.0e-5f;

void CheckVec3(Vec3 actual, Vec3 expected, float tol = kTol)
{
    CHECK(IsNearlyEqual(actual.X(), expected.X(), tol));
    CHECK(IsNearlyEqual(actual.Y(), expected.Y(), tol));
    CHECK(IsNearlyEqual(actual.Z(), expected.Z(), tol));
}

void CheckVec4(Vec4 actual, Vec4 expected, float tol = kTol)
{
    CHECK(IsNearlyEqual(actual.X(), expected.X(), tol));
    CHECK(IsNearlyEqual(actual.Y(), expected.Y(), tol));
    CHECK(IsNearlyEqual(actual.Z(), expected.Z(), tol));
    CHECK(IsNearlyEqual(actual.W(), expected.W(), tol));
}

} // namespace

// ---- Vec3 -----------------------------------------------------------------

TEST_CASE("Vec3: default + xyz + splat + Raw construction")
{
    const Vec3 zero{};
    CHECK(zero.X() == 0.0f); CHECK(zero.Y() == 0.0f); CHECK(zero.Z() == 0.0f);

    const Vec3 v{1.0f, 2.0f, 3.0f};
    CHECK(v.X() == 1.0f); CHECK(v.Y() == 2.0f); CHECK(v.Z() == 3.0f);

    const Vec3 splat(5.0f);
    CHECK(splat.X() == 5.0f); CHECK(splat.Y() == 5.0f); CHECK(splat.Z() == 5.0f);

    const Vec3 raw{SimdSet(7.0f, 8.0f, 9.0f, 0.0f)};
    CHECK(raw.X() == 7.0f);
}

TEST_CASE("Vec3: factory constants Zero/One/UnitX/UnitY/UnitZ")
{
    CheckVec3(Vec3::Zero(),  Vec3{0.0f, 0.0f, 0.0f});
    CheckVec3(Vec3::One(),   Vec3{1.0f, 1.0f, 1.0f});
    CheckVec3(Vec3::UnitX(), Vec3{1.0f, 0.0f, 0.0f});
    CheckVec3(Vec3::UnitY(), Vec3{0.0f, 1.0f, 0.0f});
    CheckVec3(Vec3::UnitZ(), Vec3{0.0f, 0.0f, 1.0f});
}

TEST_CASE("Vec3: Vec3Packed roundtrip preserves xyz")
{
    const Vec3 v{1.5f, -2.25f, 7.75f};
    const Vec3Packed packed = v.ToPacked();
    CHECK(packed.x_ == 1.5f);
    CHECK(packed.y_ == -2.25f);
    CHECK(packed.z_ == 7.75f);
    CheckVec3(Vec3::FromPacked(packed), v);
    static_assert(sizeof(Vec3Packed) == 12);
}

TEST_CASE("Vec3: arithmetic operators")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, 5.0f, 6.0f};
    CheckVec3(a + b, Vec3{5.0f, 7.0f, 9.0f});
    CheckVec3(a - b, Vec3{-3.0f, -3.0f, -3.0f});
    CheckVec3(a * b, Vec3{4.0f, 10.0f, 18.0f});
    CheckVec3(a * 2.0f, Vec3{2.0f, 4.0f, 6.0f});
    CheckVec3(2.0f * a, Vec3{2.0f, 4.0f, 6.0f});
    CheckVec3(a / 2.0f, Vec3{0.5f, 1.0f, 1.5f});
    CheckVec3(-a, Vec3{-1.0f, -2.0f, -3.0f});
}

TEST_CASE("Vec3::Dot / Cross / Length")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, -5.0f, 6.0f};
    CHECK(IsNearlyEqual(Vec3::Dot(a, b), 4.0f - 10.0f + 18.0f));
    CHECK(IsNearlyEqual(Vec3::Dot(a, a), a.LengthSquared()));

    CheckVec3(Vec3::Cross(Vec3::UnitX(), Vec3::UnitY()), Vec3::UnitZ());
    CheckVec3(Vec3::Cross(Vec3::UnitY(), Vec3::UnitZ()), Vec3::UnitX());
    CheckVec3(Vec3::Cross(Vec3::UnitZ(), Vec3::UnitX()), Vec3::UnitY());
    CheckVec3(Vec3::Cross(Vec3::UnitX(), Vec3::UnitX()), Vec3::Zero());

    CHECK(IsNearlyEqual(a.LengthSquared(), 14.0f));
    CHECK(IsNearlyEqual(a.Length(), std::sqrt(14.0f)));
}

TEST_CASE("Vec3::Normalized: unit length for non-zero; zero for zero-length")
{
    CheckVec3(Vec3{3.0f, 0.0f, 4.0f}.Normalized(), Vec3{0.6f, 0.0f, 0.8f});
    CHECK(IsNearlyEqual(Vec3{1.0f, 2.0f, 3.0f}.Normalized().Length(), 1.0f));
    CheckVec3(Vec3::Zero().Normalized(), Vec3::Zero());
    CheckVec3(Vec3(0.5f * kEpsilon).Normalized(), Vec3::Zero());
}

TEST_CASE("Vec3::Min / Max / Lerp")
{
    const Vec3 a{1.0f, 4.0f, -1.0f};
    const Vec3 b{3.0f, 2.0f, 0.0f};
    CheckVec3(Vec3::Min(a, b), Vec3{1.0f, 2.0f, -1.0f});
    CheckVec3(Vec3::Max(a, b), Vec3{3.0f, 4.0f, 0.0f});
    CheckVec3(Vec3::Lerp(a, b, 0.0f), a);
    CheckVec3(Vec3::Lerp(a, b, 1.0f), b);
    CheckVec3(Vec3::Lerp(a, b, 0.5f), Vec3{2.0f, 3.0f, -0.5f});
}

TEST_CASE("Vec3::operator== / != ignore the SIMD W lane")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    // Manually craft a Vec3 with a non-zero W lane via SimdSet.
    const Vec3 b{SimdSet(1.0f, 2.0f, 3.0f, 99.0f)};
    CHECK(a == b);
    CHECK_FALSE(a != b);
    CHECK(Vec3{1.0f, 2.0f, 3.0f} != Vec3{1.0f, 2.0f, 4.0f});
}

TEST_CASE("Vec3: size & alignment static_asserts")
{
    static_assert(sizeof(Vec3) == 16);
    static_assert(alignof(Vec3) == 16);
}

// ---- Vec4 -----------------------------------------------------------------

TEST_CASE("Vec4: default + xyzw + splat + Raw construction")
{
    const Vec4 zero{};
    CHECK(zero.X() == 0.0f); CHECK(zero.Y() == 0.0f);
    CHECK(zero.Z() == 0.0f); CHECK(zero.W() == 0.0f);

    const Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(v.X() == 1.0f); CHECK(v.W() == 4.0f);

    const Vec4 splat(3.0f);
    CheckVec4(splat, Vec4{3.0f, 3.0f, 3.0f, 3.0f});

    const Vec4 raw{SimdSet(5.0f, 6.0f, 7.0f, 8.0f)};
    CheckVec4(raw, Vec4{5.0f, 6.0f, 7.0f, 8.0f});
}

TEST_CASE("Vec4: arithmetic operators")
{
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{2.0f, 0.0f, -1.0f, 0.5f};
    CheckVec4(a + b, Vec4{3.0f, 2.0f, 2.0f, 4.5f});
    CheckVec4(a - b, Vec4{-1.0f, 2.0f, 4.0f, 3.5f});
    CheckVec4(a * b, Vec4{2.0f, 0.0f, -3.0f, 2.0f});
    CheckVec4(a / Vec4{1.0f, 2.0f, 3.0f, 4.0f}, Vec4{1.0f, 1.0f, 1.0f, 1.0f});
    CheckVec4(a * 2.0f, Vec4{2.0f, 4.0f, 6.0f, 8.0f});
    CheckVec4(2.0f * a, Vec4{2.0f, 4.0f, 6.0f, 8.0f});
    CheckVec4(a / 2.0f, Vec4{0.5f, 1.0f, 1.5f, 2.0f});
    CheckVec4(-a, Vec4{-1.0f, -2.0f, -3.0f, -4.0f});
}

TEST_CASE("Vec4: Dot / LengthSquared / Length")
{
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{2.0f, 0.0f, -1.0f, 1.0f};
    CHECK(IsNearlyEqual(Vec4::Dot(a, b), 2.0f + 0.0f - 3.0f + 4.0f));
    CHECK(IsNearlyEqual(a.LengthSquared(), 30.0f));
    CHECK(IsNearlyEqual(a.Length(), std::sqrt(30.0f)));
}

TEST_CASE("Vec4::Normalized: unit length for non-zero; zero for zero-length")
{
    CHECK(IsNearlyEqual(Vec4{1.0f, 2.0f, 3.0f, 4.0f}.Normalized().Length(), 1.0f));
    CheckVec4(Vec4{}.Normalized(), Vec4{});
    CheckVec4(Vec4(0.5f * kEpsilon).Normalized(), Vec4{});
}

TEST_CASE("Vec4::Min / Max / Lerp")
{
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{2.0f, 0.0f, -1.0f, 1.0f};
    CheckVec4(Vec4::Lerp(a, b, 0.0f), a);
    CheckVec4(Vec4::Lerp(a, b, 1.0f), b);
    CheckVec4(Vec4::Min(a, b), Vec4{1.0f, 0.0f, -1.0f, 1.0f});
    CheckVec4(Vec4::Max(a, b), Vec4{2.0f, 2.0f, 3.0f, 4.0f});
}

TEST_CASE("Vec4: aligned / unaligned load + store roundtrip")
{
    alignas(16) float src[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float dst[4] = {};
    Vec4::LoadAligned(src).StoreAligned(dst);
    CHECK(dst[0] == 1.0f); CHECK(dst[1] == 2.0f);
    CHECK(dst[2] == 3.0f); CHECK(dst[3] == 4.0f);

    float uSrc[5] = {0.0f, 10.0f, 20.0f, 30.0f, 40.0f};
    float uDst[5] = {};
    Vec4::LoadUnaligned(uSrc + 1).StoreUnaligned(uDst + 1);
    CHECK(uDst[1] == 10.0f);
    CHECK(uDst[4] == 40.0f);
}

TEST_CASE("Vec4::operator== / != include the W lane")
{
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 c{1.0f, 2.0f, 3.0f, 5.0f};
    CHECK(a == b);
    CHECK(a != c);
    CHECK_FALSE(a != b);
}

TEST_CASE("Vec4: size & alignment static_asserts")
{
    static_assert(sizeof(Vec4) == 16);
    static_assert(alignof(Vec4) == 16);
}

TEST_SUITE_END();
