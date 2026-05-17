/// @file
/// @brief Tests for Mat3, Mat3x4, and Mat4.

#include "Core/Math/Common.h"
#include "Core/Math/Mat3.h"
#include "Core/Math/Mat3x4.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Math");

#include <cmath>

using namespace Hylux::Math;

namespace
{

constexpr float kTol = 1.0e-4f;

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

// ---- Mat3 ------------------------------------------------------------------

TEST_CASE("Mat3: default + Identity construct identity; Column accessor")
{
    const Mat3 id;
    CheckVec3(id.Column(0), Vec3::UnitX());
    CheckVec3(id.Column(1), Vec3::UnitY());
    CheckVec3(id.Column(2), Vec3::UnitZ());
    CheckVec3(Mat3::Identity().Column(2), Vec3::UnitZ());
}

TEST_CASE("Mat3: column constructor stores columns")
{
    const Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    CheckVec3(m.Column(0), Vec3{1, 2, 3});
    CheckVec3(m.Column(1), Vec3{4, 5, 6});
    CheckVec3(m.Column(2), Vec3{7, 8, 9});
}

TEST_CASE("Mat3::FromColumnsRaw matches Column accessor")
{
    const Mat3 m = Mat3::FromColumnsRaw(SimdSet(1, 0, 0, 0), SimdSet(0, 1, 0, 0), SimdSet(0, 0, 1, 0));
    CheckVec3(m.Column(0), Vec3::UnitX());
    CheckVec3(m.Column(1), Vec3::UnitY());
}

TEST_CASE("Mat3 * Vec3: identity is neutral; rotation matrix produces expected result")
{
    const Mat3 id;
    CheckVec3(id * Vec3{2, 3, 4}, Vec3{2, 3, 4});

    // 90deg rotation around Z (column-vector convention).
    const Mat3 rotZ{Vec3{0, 1, 0}, Vec3{-1, 0, 0}, Vec3{0, 0, 1}};
    CheckVec3(rotZ * Vec3::UnitX(), Vec3::UnitY());
    CheckVec3(rotZ * Vec3::UnitY(), Vec3{-1, 0, 0});
}

TEST_CASE("Mat3 * Mat3: identity neutral on both sides; composition is correct")
{
    const Mat3 a{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    const Mat3 id;
    CheckVec3((a * id).Column(0), a.Column(0));
    CheckVec3((id * a).Column(2), a.Column(2));
}

TEST_CASE("Mat3::Transposed: involutive and swaps rows/cols")
{
    const Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    const Mat3 t = m.Transposed();
    CheckVec3(t.Column(0), Vec3{1, 4, 7});
    CheckVec3(t.Column(1), Vec3{2, 5, 8});
    CheckVec3(t.Column(2), Vec3{3, 6, 9});
    for (int j = 0; j < 3; ++j)
    {
        CheckVec3(t.Transposed().Column(j), m.Column(j));
    }
}

TEST_CASE("Mat3: size & alignment")
{
    static_assert(sizeof(Mat3) == 48);
    static_assert(alignof(Mat3) == 16);
}

// ---- Mat3x4 ---------------------------------------------------------------

TEST_CASE("Mat3x4: default + Identity construct identity rows; TransformPoint/Dir are identity")
{
    const Mat3x4 id;
    CheckVec3(id.TransformPoint(Vec3{1, 2, 3}), Vec3{1, 2, 3});
    CheckVec3(id.TransformDir(Vec3{1, 0, 0}), Vec3{1, 0, 0});
    CheckVec3(Mat3x4::Identity().TransformPoint(Vec3::Zero()), Vec3::Zero());
}

TEST_CASE("Mat3x4: row constructor stores rows")
{
    const Mat3x4 m{Vec4{1, 0, 0, 10}, Vec4{0, 1, 0, 20}, Vec4{0, 0, 1, 30}};
    CheckVec4(m.Row(0), Vec4{1, 0, 0, 10});
    CheckVec4(m.Row(1), Vec4{0, 1, 0, 20});
    CheckVec4(m.Row(2), Vec4{0, 0, 1, 30});
}

TEST_CASE("Mat3x4: TransformPoint includes translation; TransformDir does not")
{
    const Mat3x4 m{Vec4{1, 0, 0, 10}, Vec4{0, 1, 0, 20}, Vec4{0, 0, 1, 30}};
    CheckVec3(m.TransformPoint(Vec3{1, 1, 1}), Vec3{11, 21, 31});
    CheckVec3(m.TransformDir(Vec3{1, 1, 1}), Vec3{1, 1, 1});
}

TEST_CASE("Mat3x4 * Vec3 alias of TransformPoint")
{
    const Mat3x4 m{Vec4{2, 0, 0, 5}, Vec4{0, 2, 0, 6}, Vec4{0, 0, 2, 7}};
    CheckVec3(m * Vec3{1, 1, 1}, Vec3{7, 8, 9});
}

TEST_CASE("Mat3x4::FromMat4: identity Mat4 -> identity Mat3x4")
{
    const Mat3x4 m = Mat3x4::FromMat4(Mat4::Identity());
    CheckVec3(m.TransformPoint(Vec3{1, 2, 3}), Vec3{1, 2, 3});
}

TEST_CASE("Mat3x4::FromMat4: translation pulled into the W column")
{
    const Mat4 t = Mat4::Translation(Vec3{10, 20, 30});
    const Mat3x4 m = Mat3x4::FromMat4(t);
    CheckVec4(m.Row(0), Vec4{1, 0, 0, 10});
    CheckVec4(m.Row(1), Vec4{0, 1, 0, 20});
    CheckVec4(m.Row(2), Vec4{0, 0, 1, 30});
}

TEST_CASE("Mat3x4::FromRowsRaw matches Row accessor")
{
    const Mat3x4 m = Mat3x4::FromRowsRaw(SimdSet(1, 0, 0, 0), SimdSet(0, 1, 0, 0), SimdSet(0, 0, 1, 0));
    CheckVec4(m.Row(0), Vec4{1, 0, 0, 0});
}

TEST_CASE("Mat3x4::StoreRowMajorAligned / StoreRowMajor write 12 floats")
{
    const Mat3x4 m{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}};
    alignas(16) float aligned[12] = {};
    m.StoreRowMajorAligned(aligned);
    CHECK(aligned[0] == 1.0f); CHECK(aligned[3] == 4.0f);
    CHECK(aligned[4] == 5.0f); CHECK(aligned[11] == 12.0f);

    float unaligned[13] = {};
    m.StoreRowMajor(unaligned + 1);
    CHECK(unaligned[1] == 1.0f);
    CHECK(unaligned[12] == 12.0f);
}

TEST_CASE("Mat3x4: size & alignment")
{
    static_assert(sizeof(Mat3x4) == 48);
    static_assert(alignof(Mat3x4) == 16);
}

// ---- Mat4 -----------------------------------------------------------------

TEST_CASE("Mat4: default + Identity factory")
{
    const Mat4 id;
    CheckVec4(id.Column(0), Vec4{1, 0, 0, 0});
    CheckVec4(id.Column(1), Vec4{0, 1, 0, 0});
    CheckVec4(id.Column(2), Vec4{0, 0, 1, 0});
    CheckVec4(id.Column(3), Vec4{0, 0, 0, 1});
    CheckVec4(Mat4::Identity().Column(3), Vec4{0, 0, 0, 1});
}

TEST_CASE("Mat4: column constructor preserves columns")
{
    const Mat4 m{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    CheckVec4(m.Column(0), Vec4{1, 2, 3, 4});
    CheckVec4(m.Column(3), Vec4{13, 14, 15, 16});
}

TEST_CASE("Mat4::Scale produces diagonal scale matrix")
{
    const Mat4 s = Mat4::Scale(Vec3{2, 3, 4});
    CheckVec4(s * Vec4{1, 1, 1, 1}, Vec4{2, 3, 4, 1});
    CheckVec4(s.Column(3), Vec4{0, 0, 0, 1});
}

TEST_CASE("Mat4::Translation: leaves basis identity, sets W column")
{
    const Mat4 t = Mat4::Translation(Vec3{10, 20, 30});
    CheckVec4(t.Column(0), Vec4{1, 0, 0, 0});
    CheckVec4(t.Column(3), Vec4{10, 20, 30, 1});
    CheckVec4(t * Vec4{1, 2, 3, 1}, Vec4{11, 22, 33, 1});
    // direction (w=0) unaffected
    CheckVec4(t * Vec4{1, 0, 0, 0}, Vec4{1, 0, 0, 0});
}

TEST_CASE("Mat4 * Mat4: identity neutral; composition is TR")
{
    const Mat4 t = Mat4::Translation(Vec3{1, 2, 3});
    const Mat4 s = Mat4::Scale(Vec3{2, 2, 2});
    // (T * S) * v should scale then translate.
    CheckVec4((t * s) * Vec4{1, 1, 1, 1}, Vec4{3, 4, 5, 1});
    // identity is neutral
    CheckVec4((t * Mat4::Identity()).Column(3), t.Column(3));
}

TEST_CASE("Mat4::Transposed is involutive and swaps rows/cols")
{
    const Mat4 m = Mat4::Translation(Vec3{1, 2, 3}) * Mat4::Scale(Vec3{2, 3, 4});
    const Mat4 t = m.Transposed();
    const Mat4 t2 = t.Transposed();
    for (int j = 0; j < 4; ++j)
    {
        CheckVec4(t2.Column(j), m.Column(j));
    }
}

TEST_CASE("Mat4::Inverse: TRS roundtrip is identity")
{
    const Mat4 m = Mat4::Translation(Vec3{1, -2, 3}) * Mat4::Scale(Vec3{2, 0.5f, -3});
    const Mat4 inv = m.Inverse();
    const Mat4 prod = inv * m;
    for (int j = 0; j < 4; ++j)
    {
        CheckVec4(prod.Column(j), Mat4::Identity().Column(j));
    }
}

TEST_CASE("Mat4::Inverse: singular falls back to identity")
{
    const Mat4 zero{Vec4{0, 0, 0, 0}, Vec4{0, 0, 0, 0}, Vec4{0, 0, 0, 0}, Vec4{0, 0, 0, 0}};
    const Mat4 inv = zero.Inverse();
    for (int j = 0; j < 4; ++j)
    {
        CheckVec4(inv.Column(j), Mat4::Identity().Column(j));
    }
}

TEST_CASE("Mat4::PerspectiveRH_ZO maps near->0 and far->1")
{
    const float n = 0.5f, f = 100.0f;
    const Mat4 p = Mat4::PerspectiveRH_ZO(kHalfPi, 16.0f / 9.0f, n, f);
    const Vec4 pn = p * Vec4{0, 0, -n, 1};
    const Vec4 pf = p * Vec4{0, 0, -f, 1};
    CHECK(IsNearlyEqual(pn.Z() / pn.W(), 0.0f, kTol));
    CHECK(IsNearlyEqual(pf.Z() / pf.W(), 1.0f, kTol));
}

TEST_CASE("Mat4::OrthoRH_ZO maps the right/top/near corners to NDC corners")
{
    const Mat4 o = Mat4::OrthoRH_ZO(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f);
    CheckVec4(o * Vec4{1, 1, 0, 1}, Vec4{1, 1, 0, 1});
    const Vec4 farPoint = o * Vec4{0, 0, -100.0f, 1};
    CHECK(IsNearlyEqual(farPoint.Z() / farPoint.W(), 1.0f, kTol));
}

TEST_CASE("Mat4::LookAtRH places the camera at eye looking toward target")
{
    const Mat4 v = Mat4::LookAtRH(Vec3{0, 0, 5}, Vec3{0, 0, 0}, Vec3::UnitY());
    // The eye must transform to the origin in view space.
    CheckVec4(v * Vec4{0, 0, 5, 1}, Vec4{0, 0, 0, 1});
}

TEST_CASE("Mat4::FromColumnsRaw matches Column accessor")
{
    const Mat4 m = Mat4::FromColumnsRaw(SimdSet(1, 0, 0, 0), SimdSet(0, 1, 0, 0),
                                        SimdSet(0, 0, 1, 0), SimdSet(0, 0, 0, 1));
    CheckVec4(m.Column(0), Vec4{1, 0, 0, 0});
    CheckVec4(m.Column(3), Vec4{0, 0, 0, 1});
}

TEST_CASE("Mat4: size & alignment")
{
    static_assert(sizeof(Mat4) == 64);
    static_assert(alignof(Mat4) == 16);
}

TEST_SUITE_END();
