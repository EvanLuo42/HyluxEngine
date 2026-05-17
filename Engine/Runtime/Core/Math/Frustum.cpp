/// @file
/// @brief Frustum implementation.

#include "Core/Math/Frustum.h"

#include "Core/Math/Common.h"

#include <cmath>

namespace Hylux::Math
{
namespace
{

/// @brief Returns (nx, ny, nz, d) normalized by |n|. Drops the plane when |n| underflows.
[[nodiscard]] Vec4 NormalizePlane(Vec4 raw) noexcept
{
    const float lenSq = raw.X() * raw.X() + raw.Y() * raw.Y() + raw.Z() * raw.Z();
    if (lenSq <= kEpsilon)
    {
        return Vec4{0.0f, 0.0f, 0.0f, 0.0f};
    }
    const float invLen = 1.0f / std::sqrt(lenSq);
    return Vec4{raw.X() * invLen, raw.Y() * invLen, raw.Z() * invLen, raw.W() * invLen};
}

} // namespace

Frustum Frustum::FromViewProj(const Mat4& viewProj) noexcept
{
    // viewProj is column-major: Column(j) returns (M[0][j], M[1][j], M[2][j], M[3][j]).
    // We need the four rows of the matrix as Vec4s.
    const Vec4 c0 = viewProj.Column(0);
    const Vec4 c1 = viewProj.Column(1);
    const Vec4 c2 = viewProj.Column(2);
    const Vec4 c3 = viewProj.Column(3);

    const Vec4 row0{c0.X(), c1.X(), c2.X(), c3.X()};
    const Vec4 row1{c0.Y(), c1.Y(), c2.Y(), c3.Y()};
    const Vec4 row2{c0.Z(), c1.Z(), c2.Z(), c3.Z()};
    const Vec4 row3{c0.W(), c1.W(), c2.W(), c3.W()};

    // Gribb-Hartmann extraction. Vulkan / D3D12 NDC: x,y in [-1, 1], z in [0, 1].
    // Note Near uses row2 alone (not row3 + row2) because z_ndc >= 0 (not -1).
    Frustum result;
    result.planes_[Left]   = NormalizePlane(row3 + row0);
    result.planes_[Right]  = NormalizePlane(row3 - row0);
    result.planes_[Bottom] = NormalizePlane(row3 + row1);
    result.planes_[Top]    = NormalizePlane(row3 - row1);
    result.planes_[Near]   = NormalizePlane(row2);
    result.planes_[Far]    = NormalizePlane(row3 - row2);
    return result;
}

bool Frustum::ContainsPoint(Vec3 point) const noexcept
{
    for (std::uint32_t i = 0; i < Count; ++i)
    {
        const Vec4& plane = planes_[i];
        const float signedDistance =
            plane.X() * point.X() + plane.Y() * point.Y() + plane.Z() * point.Z() + plane.W();
        if (signedDistance < 0.0f)
        {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsSphere(Vec3 center, float radius) const noexcept
{
    for (std::uint32_t i = 0; i < Count; ++i)
    {
        const Vec4& plane = planes_[i];
        const float signedDistance =
            plane.X() * center.X() + plane.Y() * center.Y() + plane.Z() * center.Z() + plane.W();
        if (signedDistance < -radius)
        {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsAabb(Vec3 boxMin, Vec3 boxMax) const noexcept
{
    for (std::uint32_t i = 0; i < Count; ++i)
    {
        const Vec4& plane = planes_[i];
        // Pick the AABB vertex farthest along the plane normal (the "positive vertex").
        // If even that vertex sits on the negative side, the entire box is outside.
        const float px = plane.X() >= 0.0f ? boxMax.X() : boxMin.X();
        const float py = plane.Y() >= 0.0f ? boxMax.Y() : boxMin.Y();
        const float pz = plane.Z() >= 0.0f ? boxMax.Z() : boxMin.Z();
        const float signedDistance = plane.X() * px + plane.Y() * py + plane.Z() * pz + plane.W();
        if (signedDistance < 0.0f)
        {
            return false;
        }
    }
    return true;
}

} // namespace Hylux::Math
