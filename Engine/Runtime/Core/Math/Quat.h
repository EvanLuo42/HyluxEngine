/// @file
/// @brief Unit quaternion (x, y, z, w) for 3D rotations. SIMD-backed storage.

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Simd.h"
#include "Core/Math/Vec3.h"

namespace Hylux::Math
{

class Mat3;
class Mat4;

/// @brief Quaternion in (x, y, z, w) order. Identity by default.
class HYLUX_ALIGN(16) Quat
{
public:
    HYLUX_FORCEINLINE Quat() noexcept : v_(SimdSet(0.0f, 0.0f, 0.0f, 1.0f)) {}
    HYLUX_FORCEINLINE Quat(float x, float y, float z, float w) noexcept : v_(SimdSet(x, y, z, w)) {}
    HYLUX_FORCEINLINE explicit Quat(SimdF32x4 raw) noexcept : v_(raw) {}

    [[nodiscard]] HYLUX_FORCEINLINE float X() const noexcept { return SimdGetX(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Y() const noexcept { return SimdGetY(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Z() const noexcept { return SimdGetZ(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float W() const noexcept { return SimdGetW(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 Raw() const noexcept { return v_; }

    [[nodiscard]] static HYLUX_FORCEINLINE Quat Identity() noexcept { return Quat{}; }

    /// @brief Builds a unit quaternion that rotates angleRadians around the given axis.
    /// @param axis must be unit length; behavior is undefined otherwise.
    [[nodiscard]] static Quat FromAxisAngle(Vec3 axis, float angleRadians) noexcept;

    /// @brief Builds from intrinsic Tait-Bryan angles (yaw=Y, pitch=X, roll=Z), applied Y * X * Z.
    [[nodiscard]] static Quat FromEuler(float pitchX, float yawY, float rollZ) noexcept;

    /// @brief Spherical linear interpolation. Result is always normalized.
    [[nodiscard]] static Quat Slerp(Quat a, Quat b, float t) noexcept;

    /// @brief Conjugate (negates xyz, keeps w). For unit quaternions equals the inverse.
    [[nodiscard]] HYLUX_FORCEINLINE Quat Conjugate() const noexcept
    {
        return Quat{SimdMul(v_, SimdSet(-1.0f, -1.0f, -1.0f, 1.0f))};
    }

    /// @brief Rotates v by this quaternion.
    [[nodiscard]] Vec3 Rotate(Vec3 v) const noexcept;

    /// @brief Returns the equivalent 3x3 rotation matrix.
    [[nodiscard]] Mat3 ToMat3() const noexcept;

    /// @brief Returns the equivalent 4x4 rotation matrix (translation = 0).
    [[nodiscard]] Mat4 ToMat4() const noexcept;

    /// @brief Renormalizes the quaternion. Cheap protection against drift.
    [[nodiscard]] HYLUX_FORCEINLINE Quat Normalized() const noexcept
    {
        const float len = std::sqrt(SimdGetX(SimdDot4(v_, v_)));
        if (len <= kEpsilon)
        {
            return Quat::Identity();
        }
        return Quat{SimdMul(v_, SimdSplat(1.0f / len))};
    }

private:
    SimdF32x4 v_;
};

/// @brief Quaternion product (q1 * q2). Right-applies q2 first, then q1
///        (matching column-vector convention used by Mat3/Mat4).
[[nodiscard]] Quat operator*(Quat a, Quat b) noexcept;

static_assert(sizeof(Quat) == 16, "Quat must be 16 bytes");
static_assert(alignof(Quat) == 16, "Quat must be 16-byte aligned");

} // namespace Hylux::Math
