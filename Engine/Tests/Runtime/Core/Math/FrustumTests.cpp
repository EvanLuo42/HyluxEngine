/// @file
/// @brief Tests for Hylux::Math::Frustum (Gribb-Hartmann extraction + point/sphere/AABB tests).

#include "Core/Math/Common.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Vec3.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Math");

using namespace Hylux::Math;

TEST_CASE("Frustum: default-constructed has all-zero planes; contains anything (signedDistance = 0)")
{
    const Frustum f;
    CHECK(f.GetPlane(Frustum::Left) == Vec4{0, 0, 0, 0});
    CHECK(f.GetPlane(Frustum::Far) == Vec4{0, 0, 0, 0});
    // With every plane (0,0,0,0), the signed distance is 0 for every point, which the
    // "inside" test treats as on-plane (>= 0) -> point is inside.
    CHECK(f.ContainsPoint(Vec3{0, 0, 0}));
    CHECK(f.ContainsPoint(Vec3{1e9f, -1e9f, 1e9f}));
    CHECK(f.IntersectsSphere(Vec3{0, 0, 0}, 1.0f));
    CHECK(f.IntersectsAabb(Vec3{-1, -1, -1}, Vec3{1, 1, 1}));
}

TEST_CASE("Frustum::FromViewProj: standard perspective camera contains origin-relative points")
{
    const Mat4 view = Mat4::LookAtRH(Vec3{0, 0, 5}, Vec3{0, 0, 0}, Vec3::UnitY());
    const Mat4 proj = Mat4::PerspectiveRH_ZO(kHalfPi, 1.0f, 0.1f, 100.0f);
    const Mat4 vp   = proj * view;
    const Frustum f = Frustum::FromViewProj(vp);

    CHECK(f.ContainsPoint(Vec3{0, 0, 0}));
    // Point far behind the camera is outside.
    CHECK_FALSE(f.ContainsPoint(Vec3{0, 0, 1000}));
}

TEST_CASE("Frustum::IntersectsSphere: radius extends inclusion across planes")
{
    const Mat4 view = Mat4::LookAtRH(Vec3{0, 0, 5}, Vec3{0, 0, 0}, Vec3::UnitY());
    const Mat4 proj = Mat4::PerspectiveRH_ZO(kHalfPi, 1.0f, 0.1f, 100.0f);
    const Frustum f = Frustum::FromViewProj(proj * view);

    CHECK(f.IntersectsSphere(Vec3{0, 0, 0}, 0.1f));
    CHECK(f.IntersectsSphere(Vec3{0, 0, 1000}, 1e6f));
    CHECK_FALSE(f.IntersectsSphere(Vec3{0, 0, 1000}, 0.1f));
}

TEST_CASE("Frustum::IntersectsAabb: degenerate AABB reduces to a point test")
{
    const Mat4 view = Mat4::LookAtRH(Vec3{0, 0, 5}, Vec3{0, 0, 0}, Vec3::UnitY());
    const Mat4 proj = Mat4::PerspectiveRH_ZO(kHalfPi, 1.0f, 0.1f, 100.0f);
    const Frustum f = Frustum::FromViewProj(proj * view);

    CHECK(f.IntersectsAabb(Vec3{0, 0, 0}, Vec3{0, 0, 0}));
    CHECK_FALSE(f.IntersectsAabb(Vec3{0, 0, 1000}, Vec3{0.1f, 0.1f, 1001.0f}));
    // Box straddling the far plane still passes the positive-vertex test.
    CHECK(f.IntersectsAabb(Vec3{-1, -1, -1}, Vec3{1, 1, 200}));
}

TEST_CASE("Frustum: PlaneIndex enumerators are 0..5 with Count==6")
{
    static_assert(Frustum::Left == 0);
    static_assert(Frustum::Right == 1);
    static_assert(Frustum::Bottom == 2);
    static_assert(Frustum::Top == 3);
    static_assert(Frustum::Near == 4);
    static_assert(Frustum::Far == 5);
    static_assert(Frustum::Count == 6);
}

TEST_SUITE_END();
