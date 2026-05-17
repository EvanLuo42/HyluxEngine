/// @file
/// @brief Tests for Math::Mat3x4 — identity, translation, FromMat4, StoreRowMajor.

#include "Core/Math/Common.h"
#include "Core/Math/Mat3x4.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <doctest/doctest.h>

#include <array>

using namespace Hylux::Math;

namespace
{

constexpr float kTol = 1.0e-5f;

void CheckVec3Near(Vec3 a, Vec3 b, float tol = kTol)
{
    CHECK(IsNearlyEqual(a.X(), b.X(), tol));
    CHECK(IsNearlyEqual(a.Y(), b.Y(), tol));
    CHECK(IsNearlyEqual(a.Z(), b.Z(), tol));
}

} // namespace

TEST_CASE("Mat3x4 identity transforms points and directions unchanged")
{
    const Mat3x4 m = Mat3x4::Identity();
    CheckVec3Near(m.TransformPoint(Vec3{1.0f, 2.0f, 3.0f}), Vec3{1.0f, 2.0f, 3.0f});
    CheckVec3Near(m.TransformDir(Vec3{0.0f, 0.0f, 1.0f}), Vec3{0.0f, 0.0f, 1.0f});
}

TEST_CASE("Mat3x4 pure translation moves points but leaves directions alone")
{
    const Vec3 t{4.0f, -2.0f, 7.0f};
    const Mat3x4 m(
        Vec4{1.0f, 0.0f, 0.0f, t.X()},
        Vec4{0.0f, 1.0f, 0.0f, t.Y()},
        Vec4{0.0f, 0.0f, 1.0f, t.Z()});

    CheckVec3Near(m.TransformPoint(Vec3{0.0f, 0.0f, 0.0f}), t);
    CheckVec3Near(m.TransformPoint(Vec3{1.0f, 1.0f, 1.0f}),
                  Vec3{1.0f + t.X(), 1.0f + t.Y(), 1.0f + t.Z()});

    // Direction transforms ignore translation (implicit w = 0).
    CheckVec3Near(m.TransformDir(Vec3{1.0f, 0.0f, 0.0f}), Vec3{1.0f, 0.0f, 0.0f});
}

TEST_CASE("Mat3x4::FromMat4 extracts the top 3 rows of a column-major Mat4")
{
    SUBCASE("identity")
    {
        const Mat3x4 m = Mat3x4::FromMat4(Mat4::Identity());
        const Vec4 r0 = m.Row(0);
        const Vec4 r1 = m.Row(1);
        const Vec4 r2 = m.Row(2);
        CHECK(r0.X() == doctest::Approx(1.0f));
        CHECK(r0.Y() == doctest::Approx(0.0f));
        CHECK(r0.Z() == doctest::Approx(0.0f));
        CHECK(r0.W() == doctest::Approx(0.0f));
        CHECK(r1.Y() == doctest::Approx(1.0f));
        CHECK(r2.Z() == doctest::Approx(1.0f));
    }

    SUBCASE("translation")
    {
        const Vec3 t{3.0f, 5.0f, -2.0f};
        const Mat3x4 m = Mat3x4::FromMat4(Mat4::Translation(t));
        // Each row's column-3 should carry the translation component.
        CHECK(m.Row(0).W() == doctest::Approx(t.X()));
        CHECK(m.Row(1).W() == doctest::Approx(t.Y()));
        CHECK(m.Row(2).W() == doctest::Approx(t.Z()));
        // TransformPoint(origin) returns the translation, same as the source Mat4.
        CheckVec3Near(m.TransformPoint(Vec3{0.0f, 0.0f, 0.0f}), t);
    }
}

TEST_CASE("Mat3x4::StoreRowMajor writes 12 floats in row-major order")
{
    const Mat3x4 m(
        Vec4{1.0f, 2.0f, 3.0f, 4.0f},
        Vec4{5.0f, 6.0f, 7.0f, 8.0f},
        Vec4{9.0f, 10.0f, 11.0f, 12.0f});

    std::array<float, 12> out{};
    m.StoreRowMajor(out.data());
    const std::array<float, 12> expected = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
    };
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        CHECK(out[i] == doctest::Approx(expected[i]));
    }
}
