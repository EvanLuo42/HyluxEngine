/// @file
/// @brief Tests for Math::Frustum — Gribb-Hartmann extraction + AABB / sphere / point tests.

#include "Core/Math/Common.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Vec3.h"

#include <doctest/doctest.h>

using namespace Hylux::Math;

namespace
{

/// @brief Builds a right-handed view-projection matrix matching the Vulkan / D3D12
///        (depth [0, 1]) clip-space convention used by Frustum::FromViewProj.
Mat4 BuildDemoViewProj()
{
    // Camera at (0, 0, 5) looking at the origin along -Z, world up = +Y.
    const Mat4 view = Mat4::LookAtRH(Vec3{0.0f, 0.0f, 5.0f},
                                     Vec3{0.0f, 0.0f, 0.0f},
                                     Vec3{0.0f, 1.0f, 0.0f});
    const Mat4 proj = Mat4::PerspectiveRH_ZO(DegToRad(90.0f), /*aspect*/ 1.0f,
                                             /*nearZ*/ 0.5f, /*farZ*/ 50.0f);
    return proj * view;
}

} // namespace

TEST_CASE("Frustum contains the origin when the camera is looking at it")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    CHECK(f.ContainsPoint(Vec3{0.0f, 0.0f, 0.0f}));
}

TEST_CASE("Frustum rejects points behind the camera")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    // Camera is at (0,0,5) looking toward -Z, so (0, 0, 10) is behind it.
    CHECK_FALSE(f.ContainsPoint(Vec3{0.0f, 0.0f, 10.0f}));
}

TEST_CASE("Frustum rejects points beyond the far plane")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    // Far plane at z = -45 from world origin (camera looks -Z from (0,0,5) with far=50).
    CHECK_FALSE(f.ContainsPoint(Vec3{0.0f, 0.0f, -100.0f}));
}

TEST_CASE("Frustum IntersectsAabb accepts the unit cube at the origin")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    CHECK(f.IntersectsAabb(Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.5f, 0.5f, 0.5f}));
}

TEST_CASE("Frustum IntersectsAabb rejects a unit cube far behind the camera")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    CHECK_FALSE(f.IntersectsAabb(Vec3{-0.5f, -0.5f,  99.5f}, Vec3{0.5f, 0.5f, 100.5f}));
}

TEST_CASE("Frustum IntersectsSphere mirrors AABB-style accept / reject")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    CHECK(f.IntersectsSphere(Vec3{0.0f, 0.0f, 0.0f}, 1.0f));
    CHECK_FALSE(f.IntersectsSphere(Vec3{0.0f, 0.0f, 100.0f}, 1.0f));
}

TEST_CASE("Frustum extraction preserves plane count and assigns left / right via Gribb-Hartmann")
{
    const Frustum f = Frustum::FromViewProj(BuildDemoViewProj());
    // Left and Right planes should differ on the X normal axis (mirror across YZ plane).
    const auto left  = f.GetPlane(Frustum::Left);
    const auto right = f.GetPlane(Frustum::Right);
    CHECK(IsNearlyEqual(left.X(), -right.X(), 1.0e-4f));
}
