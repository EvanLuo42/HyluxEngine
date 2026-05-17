/// @file
/// @brief Six-plane view frustum. Built via Gribb-Hartmann extraction from a column-major
///        view-projection matrix in Vulkan / D3D12 NDC (depth in [0, 1]). Planes are
///        normalized; tests use the standard signed-distance / positive-vertex algorithms.

#pragma once

#include "Core/Math/Mat4.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <cstdint>

namespace Hylux::Math
{

/// @brief Convention: each plane is stored as (nx, ny, nz, d) with N normalized; the
///        positive half-space (N·P + d >= 0) is "inside" the frustum.
class Frustum
{
public:
    enum PlaneIndex : std::uint32_t
    {
        Left   = 0,
        Right  = 1,
        Bottom = 2,
        Top    = 3,
        Near   = 4,
        Far    = 5,
        Count  = 6,
    };

    Frustum() = default;

    /// @brief Builds a frustum by extracting the 6 clip-space planes from a column-major
    ///        view-projection matrix (Vulkan / D3D12 [0, 1] depth convention).
    [[nodiscard]] static Frustum FromViewProj(const Mat4& viewProj) noexcept;

    [[nodiscard]] Vec4 GetPlane(PlaneIndex idx) const noexcept { return planes_[idx]; }

    /// @brief Returns true if the point lies inside (or exactly on) every plane.
    [[nodiscard]] bool ContainsPoint(Vec3 point) const noexcept;

    /// @brief Returns true if the sphere is at least partially inside every plane.
    [[nodiscard]] bool IntersectsSphere(Vec3 center, float radius) const noexcept;

    /// @brief Returns true if the axis-aligned box overlaps the frustum (positive-vertex
    ///        test — conservative; never returns false for a box that actually intersects).
    [[nodiscard]] bool IntersectsAabb(Vec3 boxMin, Vec3 boxMax) const noexcept;

private:
    Vec4 planes_[Count]{};
};

} // namespace Hylux::Math
