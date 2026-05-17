/// @file
/// @brief Tests for the Hylux math library: scalar helpers, vectors, matrices,
///        quaternions, and transforms.

#include "Core/Math/Common.h"
#include "Core/Math/Mat3.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Quat.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Vec2.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <doctest/doctest.h>

#include <cmath>

using namespace Hylux::Math;

namespace
{

constexpr float kLooseTol = 1.0e-4f;

void CheckVec3Near(Vec3 actual, Vec3 expected, float tol = kLooseTol)
{
    CHECK(IsNearlyEqual(actual.X(), expected.X(), tol));
    CHECK(IsNearlyEqual(actual.Y(), expected.Y(), tol));
    CHECK(IsNearlyEqual(actual.Z(), expected.Z(), tol));
}

void CheckVec4Near(Vec4 actual, Vec4 expected, float tol = kLooseTol)
{
    CHECK(IsNearlyEqual(actual.X(), expected.X(), tol));
    CHECK(IsNearlyEqual(actual.Y(), expected.Y(), tol));
    CHECK(IsNearlyEqual(actual.Z(), expected.Z(), tol));
    CHECK(IsNearlyEqual(actual.W(), expected.W(), tol));
}

} // namespace

// ---- scalar helpers ---------------------------------------------------------

TEST_CASE("Lerp / Clamp / Saturate behave per definition")
{
    static_assert(Lerp(0.0f, 10.0f, 0.25f) == 2.5f);
    static_assert(Clamp(5, 0, 3) == 3);
    static_assert(Clamp(-1, 0, 3) == 0);
    static_assert(Clamp(2, 0, 3) == 2);
    CHECK(Saturate(-1.0f) == 0.0f);
    CHECK(Saturate(0.5f) == 0.5f);
    CHECK(Saturate(2.0f) == 1.0f);
}

TEST_CASE("Degree <-> radian conversions round-trip through kPi")
{
    CHECK(IsNearlyEqual(DegToRad(180.0f), kPi));
    CHECK(IsNearlyEqual(RadToDeg(kPi), 180.0f));
}

TEST_CASE("SmoothStep clamps and is monotonic at the edges")
{
    CHECK(SmoothStep(0.0f, 1.0f, -1.0f) == 0.0f);
    CHECK(SmoothStep(0.0f, 1.0f, 2.0f) == 1.0f);
    CHECK(IsNearlyEqual(SmoothStep(0.0f, 1.0f, 0.5f), 0.5f));
}

// ---- Vec2 -------------------------------------------------------------------

TEST_CASE("Vec2 arithmetic and length")
{
    constexpr Vec2 a{3.0f, 4.0f};
    constexpr Vec2 b{1.0f, 2.0f};
    static_assert((a + b) == Vec2{4.0f, 6.0f});
    static_assert((a - b) == Vec2{2.0f, 2.0f});
    static_assert(Vec2::Dot(a, b) == 11.0f);
    CHECK(IsNearlyEqual(a.Length(), 5.0f));
    CHECK(IsNearlyEqual(a.Normalized().Length(), 1.0f));
}

TEST_CASE("Vec2 normalizing the zero vector yields zero (no NaN)")
{
    const Vec2 zero{0.0f, 0.0f};
    const Vec2 n = zero.Normalized();
    CHECK(n == Vec2{0.0f, 0.0f});
}

// ---- Vec3 -------------------------------------------------------------------

TEST_CASE("Vec3 dot, cross, length, and SIMD padding")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, -5.0f, 6.0f};
    CHECK(IsNearlyEqual(Vec3::Dot(a, b), 4.0f - 10.0f + 18.0f));
    CheckVec3Near(Vec3::Cross(Vec3::UnitX(), Vec3::UnitY()), Vec3::UnitZ());
    CheckVec3Near(Vec3::Cross(Vec3::UnitY(), Vec3::UnitZ()), Vec3::UnitX());
    CHECK(IsNearlyEqual(a.LengthSquared(), 14.0f));
    CHECK(IsNearlyEqual(a.Length(), std::sqrt(14.0f)));
    CHECK(IsNearlyEqual(a.Normalized().Length(), 1.0f));
}

TEST_CASE("Vec3 packed conversion round-trips xyz")
{
    const Vec3 v{1.5f, -2.25f, 7.75f};
    const Vec3Packed packed = v.ToPacked();
    CHECK(packed.x_ == 1.5f);
    CHECK(packed.y_ == -2.25f);
    CHECK(packed.z_ == 7.75f);
    CheckVec3Near(Vec3::FromPacked(packed), v);
}

TEST_CASE("Vec3 operator== ignores the SIMD W lane")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{1.0f, 2.0f, 3.0f};
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

// ---- Vec4 -------------------------------------------------------------------

TEST_CASE("Vec4 dot, lerp, and min/max")
{
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{2.0f, 0.0f, -1.0f, 1.0f};
    CHECK(IsNearlyEqual(Vec4::Dot(a, b), 2.0f + 0.0f - 3.0f + 4.0f));
    CheckVec4Near(Vec4::Lerp(a, b, 0.0f), a);
    CheckVec4Near(Vec4::Lerp(a, b, 1.0f), b);
    CheckVec4Near(Vec4::Min(a, b), Vec4{1.0f, 0.0f, -1.0f, 1.0f});
    CheckVec4Near(Vec4::Max(a, b), Vec4{2.0f, 2.0f, 3.0f, 4.0f});
}

TEST_CASE("Vec4 load/store aligned round-trips")
{
    alignas(16) float src[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float dst[4] = {};
    Vec4::LoadAligned(src).StoreAligned(dst);
    CHECK(dst[0] == 1.0f);
    CHECK(dst[1] == 2.0f);
    CHECK(dst[2] == 3.0f);
    CHECK(dst[3] == 4.0f);
}

// ---- Mat4 -------------------------------------------------------------------

TEST_CASE("Mat4 default is identity and identity * v = v")
{
    const Mat4 id;
    const Vec4 v{1.0f, 2.0f, 3.0f, 1.0f};
    CheckVec4Near(id * v, v);
}

TEST_CASE("Mat4 translation moves the translated point and preserves w")
{
    const Mat4 t = Mat4::Translation(Vec3{10.0f, 20.0f, 30.0f});
    const Vec4 point{1.0f, 2.0f, 3.0f, 1.0f};
    CheckVec4Near(t * point, Vec4{11.0f, 22.0f, 33.0f, 1.0f});

    // Direction (w = 0) should be unaffected by translation.
    const Vec4 dir{1.0f, 0.0f, 0.0f, 0.0f};
    CheckVec4Near(t * dir, dir);
}

TEST_CASE("Mat4 scale scales each axis")
{
    const Mat4 s = Mat4::Scale(Vec3{2.0f, 3.0f, 4.0f});
    const Vec4 v{1.0f, 1.0f, 1.0f, 1.0f};
    CheckVec4Near(s * v, Vec4{2.0f, 3.0f, 4.0f, 1.0f});
}

TEST_CASE("Mat4 multiplication composes left-to-right in column-vector convention")
{
    const Mat4 t = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f});
    const Mat4 s = Mat4::Scale(Vec3{2.0f, 2.0f, 2.0f});
    // (T * S) * v should first scale, then translate.
    const Vec4 v{1.0f, 1.0f, 1.0f, 1.0f};
    CheckVec4Near((t * s) * v, Vec4{3.0f, 4.0f, 5.0f, 1.0f});
}

TEST_CASE("Mat4 transposed is the involution of itself")
{
    const Mat4 m = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f}) * Mat4::Scale(Vec3{2.0f, 3.0f, 4.0f});
    const Mat4 m2 = m.Transposed().Transposed();
    for (int j = 0; j < 4; ++j)
    {
        CheckVec4Near(m2.Column(j), m.Column(j));
    }
}

TEST_CASE("Mat4 inverse times original is identity for an invertible TRS")
{
    const Mat4 m = Mat4::Translation(Vec3{1.0f, -2.0f, 3.0f}) * Mat4::Scale(Vec3{2.0f, 0.5f, -3.0f});
    const Mat4 inv = m.Inverse();
    const Mat4 prod = inv * m;
    for (int j = 0; j < 4; ++j)
    {
        const Vec4 expected = Mat4::Identity().Column(j);
        CheckVec4Near(prod.Column(j), expected);
    }
}

TEST_CASE("Mat4 PerspectiveRH_ZO maps the near and far planes to depth 0 and 1")
{
    const float n = 0.5f;
    const float f = 100.0f;
    const Mat4 p = Mat4::PerspectiveRH_ZO(kHalfPi, 16.0f / 9.0f, n, f);

    const Vec4 onNear{0.0f, 0.0f, -n, 1.0f};
    const Vec4 onFar{0.0f, 0.0f, -f, 1.0f};
    const Vec4 pn = p * onNear;
    const Vec4 pf = p * onFar;
    CHECK(IsNearlyEqual(pn.Z() / pn.W(), 0.0f, kLooseTol));
    CHECK(IsNearlyEqual(pf.Z() / pf.W(), 1.0f, kLooseTol));
}

// ---- Mat3 -------------------------------------------------------------------

TEST_CASE("Mat3 default is identity and transpose is involutive")
{
    const Mat3 id;
    CheckVec3Near(id * Vec3{1.0f, 2.0f, 3.0f}, Vec3{1.0f, 2.0f, 3.0f});
    const Mat3 t2 = id.Transposed().Transposed();
    for (int j = 0; j < 3; ++j)
    {
        CheckVec3Near(t2.Column(j), id.Column(j));
    }
}

// ---- Quat -------------------------------------------------------------------

TEST_CASE("Quat::Identity rotates everything to itself")
{
    const Quat q;
    CheckVec3Near(q.Rotate(Vec3{1.0f, 2.0f, 3.0f}), Vec3{1.0f, 2.0f, 3.0f});
}

TEST_CASE("Quat::FromAxisAngle rotates around the principal axes")
{
    const Quat qz = Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi);
    CheckVec3Near(qz.Rotate(Vec3::UnitX()), Vec3::UnitY());

    const Quat qx = Quat::FromAxisAngle(Vec3::UnitX(), kHalfPi);
    CheckVec3Near(qx.Rotate(Vec3::UnitY()), Vec3::UnitZ());

    const Quat qy = Quat::FromAxisAngle(Vec3::UnitY(), kHalfPi);
    CheckVec3Near(qy.Rotate(Vec3::UnitZ()), Vec3::UnitX());
}

TEST_CASE("Quat composition matches sequential rotation")
{
    const Quat r1 = Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi);
    const Quat r2 = Quat::FromAxisAngle(Vec3::UnitX(), kHalfPi);
    const Vec3 v{1.0f, 0.0f, 0.0f};
    // (r2 * r1) means apply r1 first, then r2.
    CheckVec3Near((r2 * r1).Rotate(v), r2.Rotate(r1.Rotate(v)));
}

TEST_CASE("Quat conjugate undoes the rotation for unit quaternions")
{
    const Quat q = Quat::FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, 0.3f);
    const Vec3 v{1.0f, 2.0f, 3.0f};
    CheckVec3Near(q.Conjugate().Rotate(q.Rotate(v)), v);
}

TEST_CASE("Quat Slerp endpoints match the inputs")
{
    const Quat a = Quat::Identity();
    const Quat b = Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi);
    const Quat s0 = Quat::Slerp(a, b, 0.0f);
    const Quat s1 = Quat::Slerp(a, b, 1.0f);
    CheckVec3Near(s0.Rotate(Vec3::UnitX()), a.Rotate(Vec3::UnitX()));
    CheckVec3Near(s1.Rotate(Vec3::UnitX()), b.Rotate(Vec3::UnitX()));
}

TEST_CASE("Quat ToMat3 agrees with direct rotation")
{
    const Quat q = Quat::FromAxisAngle(Vec3{1.0f, 1.0f, 0.0f}.Normalized(), 0.7f);
    const Mat3 m = q.ToMat3();
    const Vec3 v{0.4f, -1.2f, 2.0f};
    CheckVec3Near(m * v, q.Rotate(v));
}

// ---- Transform --------------------------------------------------------------

TEST_CASE("Transform default is identity for points and directions")
{
    const Transform t;
    CheckVec3Near(t.TransformPoint(Vec3{1.0f, 2.0f, 3.0f}), Vec3{1.0f, 2.0f, 3.0f});
    CheckVec3Near(t.TransformDirection(Vec3{1.0f, 0.0f, 0.0f}), Vec3{1.0f, 0.0f, 0.0f});
}

TEST_CASE("Transform applies scale, rotation, and translation in that order")
{
    const Transform t{
        Vec3{10.0f, 0.0f, 0.0f},
        Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi),
        Vec3{2.0f, 2.0f, 2.0f},
    };
    // Input X unit vector: scale to (2,0,0), rotate to (0,2,0), translate to (10,2,0).
    CheckVec3Near(t.TransformPoint(Vec3::UnitX()), Vec3{10.0f, 2.0f, 0.0f});
    // Direction ignores translation.
    CheckVec3Near(t.TransformDirection(Vec3::UnitX()), Vec3::UnitY());
}

TEST_CASE("Transform::ToMat4 agrees with TransformPoint")
{
    const Transform t{
        Vec3{1.0f, 2.0f, 3.0f},
        Quat::FromAxisAngle(Vec3::UnitY(), 0.3f),
        Vec3{1.5f, 1.5f, 1.5f},
    };
    const Vec3 point{0.5f, -0.25f, 1.0f};
    const Vec4 mp = t.ToMat4() * Vec4{point.X(), point.Y(), point.Z(), 1.0f};
    CheckVec3Near(Vec3{mp.X(), mp.Y(), mp.Z()}, t.TransformPoint(point));
}
