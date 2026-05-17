/// @file
/// @brief Tests for the scalar helpers in Core/Math/Common.h plus Vec2.

#include "Core/Math/Common.h"
#include "Core/Math/Vec2.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Math");

#include <cmath>
#include <limits>

using namespace Hylux::Math;

TEST_CASE("Common: scalar constants are the expected literals")
{
    CHECK(IsNearlyEqual(kPi, 3.14159265f, 1e-6f));
    CHECK(IsNearlyEqual(kTwoPi, 2.0f * kPi, 1e-6f));
    CHECK(IsNearlyEqual(kHalfPi, 0.5f * kPi, 1e-6f));
    CHECK(IsNearlyEqual(kInvPi, 1.0f / kPi, 1e-6f));
    CHECK(IsNearlyEqual(kDegToRad * 180.0f, kPi, 1e-5f));
    CHECK(IsNearlyEqual(kRadToDeg * kPi, 180.0f, 1e-3f));
}

TEST_CASE("Common::Lerp: endpoints + midpoint + unclamped extrapolation")
{
    static_assert(Lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    static_assert(Lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    static_assert(Lerp(0.0f, 10.0f, 0.5f) == 5.0f);
    CHECK(IsNearlyEqual(Lerp(-2.0f, 2.0f, -0.5f), -4.0f));
    CHECK(IsNearlyEqual(Lerp(-2.0f, 2.0f, 1.5f),  4.0f));
}

TEST_CASE("Common::Clamp: below / inside / above the range")
{
    static_assert(Clamp(-1, 0, 3) == 0);
    static_assert(Clamp(5, 0, 3) == 3);
    static_assert(Clamp(2, 0, 3) == 2);
    static_assert(Clamp(0, 0, 3) == 0);
    static_assert(Clamp(3, 0, 3) == 3);
}

TEST_CASE("Common::Saturate: clamps to [0, 1]")
{
    CHECK(Saturate(-1.0f) == 0.0f);
    CHECK(Saturate(0.0f) == 0.0f);
    CHECK(Saturate(0.5f) == 0.5f);
    CHECK(Saturate(1.0f) == 1.0f);
    CHECK(Saturate(2.0f) == 1.0f);
}

TEST_CASE("Common::DegToRad / RadToDeg: roundtrip and known angles")
{
    CHECK(IsNearlyEqual(DegToRad(0.0f), 0.0f));
    CHECK(IsNearlyEqual(DegToRad(180.0f), kPi));
    CHECK(IsNearlyEqual(RadToDeg(0.0f), 0.0f));
    CHECK(IsNearlyEqual(RadToDeg(kPi), 180.0f, 1e-3f));
    CHECK(IsNearlyEqual(DegToRad(RadToDeg(0.75f)), 0.75f, 1e-5f));
}

TEST_CASE("Common::IsNearlyEqual / IsNearlyZero: tolerance boundaries")
{
    CHECK(IsNearlyEqual(1.0f, 1.0f));
    CHECK(IsNearlyEqual(1.0f, 1.0f + 0.5f * kEpsilon));
    CHECK_FALSE(IsNearlyEqual(0.0f, 1.0f));
    CHECK(IsNearlyEqual(1.0f, 1.05f, 0.1f));
    CHECK(IsNearlyEqual(-1.0f, -1.0f));

    CHECK(IsNearlyZero(0.0f));
    CHECK(IsNearlyZero(-0.0f));
    CHECK(IsNearlyZero(0.5f * kEpsilon));
    CHECK_FALSE(IsNearlyZero(1.0f));
    CHECK(IsNearlyZero(0.05f, 0.1f));
}

TEST_CASE("Common::Square: float/int identity, sign agnostic")
{
    static_assert(Square(3) == 9);
    static_assert(Square(-4) == 16);
    static_assert(Square(0.0f) == 0.0f);
    CHECK(IsNearlyEqual(Square(2.5f), 6.25f));
}

TEST_CASE("Common::SmoothStep: clamps below/above edges; Hermite midpoint")
{
    CHECK(SmoothStep(0.0f, 1.0f, -1.0f) == 0.0f);
    CHECK(SmoothStep(0.0f, 1.0f, 2.0f) == 1.0f);
    CHECK(IsNearlyEqual(SmoothStep(0.0f, 1.0f, 0.0f), 0.0f));
    CHECK(IsNearlyEqual(SmoothStep(0.0f, 1.0f, 1.0f), 1.0f));
    CHECK(IsNearlyEqual(SmoothStep(0.0f, 1.0f, 0.5f), 0.5f));
    // Hermite curve: SmoothStep(0,1,0.25) == 3*0.0625 - 2*0.015625 = 0.15625
    CHECK(IsNearlyEqual(SmoothStep(0.0f, 1.0f, 0.25f), 0.15625f, 1e-5f));
}

// ---- Vec2 -----------------------------------------------------------------

TEST_CASE("Vec2: default / splat / explicit construction")
{
    static_assert(Vec2{} == Vec2{0.0f, 0.0f});
    static_assert(Vec2{3.0f} == Vec2{3.0f, 3.0f});
    constexpr Vec2 v{1.5f, -2.5f};
    static_assert(v.X() == 1.5f);
    static_assert(v.Y() == -2.5f);
}

TEST_CASE("Vec2: arithmetic operators (+ - * / unary-)")
{
    constexpr Vec2 a{3.0f, 4.0f};
    constexpr Vec2 b{1.0f, 2.0f};
    static_assert((a + b) == Vec2{4.0f, 6.0f});
    static_assert((a - b) == Vec2{2.0f, 2.0f});
    static_assert((a * b) == Vec2{3.0f, 8.0f});
    static_assert((a * 2.0f) == Vec2{6.0f, 8.0f});
    static_assert((2.0f * a) == Vec2{6.0f, 8.0f});
    static_assert((a / 2.0f) == Vec2{1.5f, 2.0f});
    static_assert(-a == Vec2{-3.0f, -4.0f});
    static_assert(a != b);
}

TEST_CASE("Vec2::Dot / LengthSquared / Length")
{
    constexpr Vec2 a{3.0f, 4.0f};
    constexpr Vec2 b{1.0f, 2.0f};
    static_assert(Vec2::Dot(a, b) == 11.0f);
    static_assert(a.LengthSquared() == 25.0f);
    CHECK(IsNearlyEqual(a.Length(), 5.0f));
}

TEST_CASE("Vec2::Normalized: unit length for non-zero; zero for zero-length")
{
    const Vec2 v{3.0f, 4.0f};
    CHECK(IsNearlyEqual(v.Normalized().Length(), 1.0f));
    CHECK(Vec2{0.0f, 0.0f}.Normalized() == Vec2{0.0f, 0.0f});
    CHECK(Vec2{0.5f * kEpsilon, 0.0f}.Normalized() == Vec2{0.0f, 0.0f});
}

TEST_CASE("Vec2::Min / Max / Lerp")
{
    constexpr Vec2 a{1.0f, 4.0f};
    constexpr Vec2 b{3.0f, 2.0f};
    static_assert(Vec2::Min(a, b) == Vec2{1.0f, 2.0f});
    static_assert(Vec2::Max(a, b) == Vec2{3.0f, 4.0f});
    static_assert(Vec2::Lerp(a, b, 0.0f) == a);
    static_assert(Vec2::Lerp(a, b, 1.0f) == b);
    static_assert(Vec2::Lerp(a, b, 0.5f) == Vec2{2.0f, 3.0f});
}

TEST_CASE("Vec2: size matches header static_assert")
{
    static_assert(sizeof(Vec2) == 8);
}

TEST_SUITE_END();
