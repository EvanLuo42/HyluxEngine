/// @file
/// @brief Tests for Quat + Transform.

#include "Core/Math/Common.h"
#include "Core/Math/Mat3.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Quat.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Vec3.h"

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

} // namespace

// ---- Quat -----------------------------------------------------------------

TEST_CASE("Quat: default + Identity factory == (0,0,0,1)")
{
    const Quat q;
    CHECK(q.X() == 0.0f); CHECK(q.Y() == 0.0f);
    CHECK(q.Z() == 0.0f); CHECK(q.W() == 1.0f);
    CHECK(Quat::Identity().W() == 1.0f);
}

TEST_CASE("Quat: xyzw + Raw constructors set components")
{
    const Quat q{0.1f, 0.2f, 0.3f, 0.4f};
    CHECK(q.X() == 0.1f); CHECK(q.Y() == 0.2f);
    CHECK(q.Z() == 0.3f); CHECK(q.W() == 0.4f);

    const Quat raw{SimdSet(0.5f, 0.6f, 0.7f, 0.8f)};
    CHECK(raw.W() == 0.8f);
}

TEST_CASE("Quat::FromAxisAngle: angle 0 is identity; principal axes rotate as expected")
{
    const Quat q0 = Quat::FromAxisAngle(Vec3::UnitZ(), 0.0f);
    CHECK(IsNearlyEqual(q0.W(), 1.0f));

    CheckVec3(Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi).Rotate(Vec3::UnitX()), Vec3::UnitY());
    CheckVec3(Quat::FromAxisAngle(Vec3::UnitX(), kHalfPi).Rotate(Vec3::UnitY()), Vec3::UnitZ());
    CheckVec3(Quat::FromAxisAngle(Vec3::UnitY(), kHalfPi).Rotate(Vec3::UnitZ()), Vec3::UnitX());
}

TEST_CASE("Quat::FromEuler(0,0,0) is identity; non-zero pitch rotates Y axis")
{
    const Quat q = Quat::FromEuler(0.0f, 0.0f, 0.0f);
    CHECK(IsNearlyEqual(q.W(), 1.0f));

    const Quat pitch90 = Quat::FromEuler(kHalfPi, 0.0f, 0.0f);
    // X rotation by 90 deg takes UnitY -> UnitZ.
    CheckVec3(pitch90.Rotate(Vec3::UnitY()), Vec3::UnitZ());
}

TEST_CASE("Quat: composition matches sequential rotation")
{
    const Quat r1 = Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi);
    const Quat r2 = Quat::FromAxisAngle(Vec3::UnitX(), kHalfPi);
    const Vec3 v{1, 0, 0};
    CheckVec3((r2 * r1).Rotate(v), r2.Rotate(r1.Rotate(v)));
}

TEST_CASE("Quat::Conjugate inverts the rotation for unit quaternions")
{
    const Quat q = Quat::FromAxisAngle(Vec3::UnitY(), 0.3f);
    const Vec3 v{1, 2, 3};
    CheckVec3(q.Conjugate().Rotate(q.Rotate(v)), v);
    CHECK(Quat::Identity().Conjugate().W() == 1.0f);
}

TEST_CASE("Quat::Slerp: endpoints reproduce inputs; antipodal pair takes shortest arc")
{
    const Quat a = Quat::Identity();
    const Quat b = Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi);
    CheckVec3(Quat::Slerp(a, b, 0.0f).Rotate(Vec3::UnitX()), a.Rotate(Vec3::UnitX()));
    CheckVec3(Quat::Slerp(a, b, 1.0f).Rotate(Vec3::UnitX()), b.Rotate(Vec3::UnitX()));

    // Antipodal: negate q to take the short path.
    const Quat bNeg{-b.X(), -b.Y(), -b.Z(), -b.W()};
    CheckVec3(Quat::Slerp(a, bNeg, 0.5f).Rotate(Vec3::UnitX()),
              Quat::Slerp(a, b, 0.5f).Rotate(Vec3::UnitX()));

    // Near-parallel branch: a tiny rotation falls back to LERP+normalize.
    const Quat smallRot = Quat::FromAxisAngle(Vec3::UnitZ(), 1e-7f);
    CheckVec3(Quat::Slerp(a, smallRot, 0.5f).Rotate(Vec3::UnitX()), Vec3::UnitX());
}

TEST_CASE("Quat::Normalized: zero-length quat returns Identity")
{
    const Quat zero{0.0f, 0.0f, 0.0f, 0.0f};
    const Quat n = zero.Normalized();
    CHECK(IsNearlyEqual(n.W(), 1.0f));
    CHECK(IsNearlyEqual(Quat{1.0f, 0.0f, 0.0f, 0.0f}.Normalized().X(), 1.0f));
}

TEST_CASE("Quat::Rotate: identity preserves the vector")
{
    CheckVec3(Quat::Identity().Rotate(Vec3{1.5f, -2.0f, 7.0f}), Vec3{1.5f, -2.0f, 7.0f});
}

TEST_CASE("Quat::ToMat3 / ToMat4 agree with Rotate")
{
    const Quat q = Quat::FromAxisAngle(Vec3{1, 1, 0}.Normalized(), 0.7f);
    const Mat3 m3 = q.ToMat3();
    const Mat4 m4 = q.ToMat4();
    const Vec3 v{0.4f, -1.2f, 2.0f};
    CheckVec3(m3 * v, q.Rotate(v));
    const auto m4v = m4 * Vec4{v.X(), v.Y(), v.Z(), 0.0f};
    CheckVec3(Vec3{m4v.X(), m4v.Y(), m4v.Z()}, q.Rotate(v));
    // ToMat4 translation column is (0,0,0,1).
    CHECK(IsNearlyEqual(m4.Column(3).W(), 1.0f));
}

TEST_CASE("Quat operator*: Identity neutral on both sides")
{
    const Quat q = Quat::FromAxisAngle(Vec3::UnitX(), 0.5f);
    CheckVec3((q * Quat::Identity()).Rotate(Vec3::UnitY()), q.Rotate(Vec3::UnitY()));
    CheckVec3((Quat::Identity() * q).Rotate(Vec3::UnitY()), q.Rotate(Vec3::UnitY()));
}

TEST_CASE("Quat: size & alignment")
{
    static_assert(sizeof(Quat) == 16);
    static_assert(alignof(Quat) == 16);
}

// ---- Transform ------------------------------------------------------------

TEST_CASE("Transform: default is identity for points and directions")
{
    const Transform t;
    CheckVec3(t.TransformPoint(Vec3{1, 2, 3}), Vec3{1, 2, 3});
    CheckVec3(t.TransformDirection(Vec3::UnitX()), Vec3::UnitX());
    CHECK(t.Translation() == Vec3::Zero());
    CHECK(t.Scale() == Vec3::One());
    CHECK(IsNearlyEqual(t.Rotation().W(), 1.0f));
}

TEST_CASE("Transform: getters return ctor values")
{
    const Transform t{Vec3{1, 2, 3}, Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi), Vec3{2, 2, 2}};
    CheckVec3(t.Translation(), Vec3{1, 2, 3});
    CheckVec3(t.Scale(), Vec3{2, 2, 2});
}

TEST_CASE("Transform: setters update each component independently")
{
    Transform t;
    t.SetTranslation(Vec3{5, 6, 7});
    t.SetScale(Vec3{0.5f, 0.5f, 0.5f});
    t.SetRotation(Quat::FromAxisAngle(Vec3::UnitY(), kHalfPi));
    CheckVec3(t.Translation(), Vec3{5, 6, 7});
    CheckVec3(t.Scale(), Vec3{0.5f, 0.5f, 0.5f});
    CheckVec3(t.TransformDirection(Vec3::UnitZ()), Vec3::UnitX());
}

TEST_CASE("Transform applies scale -> rotation -> translation in that order")
{
    const Transform t{Vec3{10, 0, 0}, Quat::FromAxisAngle(Vec3::UnitZ(), kHalfPi), Vec3{2, 2, 2}};
    // X unit: scale (2,0,0), rotate (0,2,0), translate (10,2,0).
    CheckVec3(t.TransformPoint(Vec3::UnitX()), Vec3{10, 2, 0});
    // direction ignores translation
    CheckVec3(t.TransformDirection(Vec3::UnitX()), Vec3::UnitY());
}

TEST_CASE("Transform::ToMat4 agrees with TransformPoint")
{
    const Transform t{Vec3{1, 2, 3}, Quat::FromAxisAngle(Vec3::UnitY(), 0.3f), Vec3{1.5f, 1.5f, 1.5f}};
    const Vec3 p{0.5f, -0.25f, 1.0f};
    const auto mp = t.ToMat4() * Vec4{p.X(), p.Y(), p.Z(), 1.0f};
    CheckVec3(Vec3{mp.X(), mp.Y(), mp.Z()}, t.TransformPoint(p));
}

TEST_CASE("Transform: zero-scale collapses TransformPoint to the translation")
{
    const Transform t{Vec3{4, 5, 6}, Quat::Identity(), Vec3::Zero()};
    CheckVec3(t.TransformPoint(Vec3{99, 99, 99}), Vec3{4, 5, 6});
}

TEST_SUITE_END();
