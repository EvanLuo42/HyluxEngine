/// @file
/// @brief 3-component float vector. Stored padded to 16 bytes so all ops can
///        run through the L0 SIMD register; the W lane is treated as 0.

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Simd.h"

namespace Hylux::Math
{

/// @brief Tightly packed 3-float storage (12 bytes). Use this in
///        vertex buffers / arrays where 4-byte padding is unacceptable.
struct Vec3Packed
{
    float x_;
    float y_;
    float z_;
};

/// @brief 3-component float vector. 16-byte aligned, SIMD-backed (W lane = 0).
class HYLUX_ALIGN(16) Vec3
{
public:
    HYLUX_FORCEINLINE Vec3() noexcept : v_(SimdZero()) {}
    HYLUX_FORCEINLINE Vec3(float x, float y, float z) noexcept : v_(SimdSet(x, y, z, 0.0f)) {}
    HYLUX_FORCEINLINE explicit Vec3(float splat) noexcept : v_(SimdSet(splat, splat, splat, 0.0f)) {}
    HYLUX_FORCEINLINE explicit Vec3(SimdF32x4 raw) noexcept : v_(raw) {}

    /// @brief Lifts a Vec3Packed into the padded SIMD representation.
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 FromPacked(Vec3Packed p) noexcept { return Vec3{p.x_, p.y_, p.z_}; }

    /// @brief Drops to tight 12-byte storage for buffer writes.
    [[nodiscard]] HYLUX_FORCEINLINE Vec3Packed ToPacked() const noexcept { return Vec3Packed{X(), Y(), Z()}; }

    [[nodiscard]] HYLUX_FORCEINLINE float X() const noexcept { return SimdGetX(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Y() const noexcept { return SimdGetY(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Z() const noexcept { return SimdGetZ(v_); }

    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 Raw() const noexcept { return v_; }

    [[nodiscard]] HYLUX_FORCEINLINE static float HYLUX_VECTORCALL Dot(Vec3 a, Vec3 b) noexcept
    {
        return SimdGetX(SimdDot3(a.v_, b.v_));
    }

    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 HYLUX_VECTORCALL Cross(Vec3 a, Vec3 b) noexcept
    {
        return Vec3{SimdCross3(a.v_, b.v_)};
    }

    [[nodiscard]] HYLUX_FORCEINLINE float LengthSquared() const noexcept { return SimdGetX(SimdDot3(v_, v_)); }

    [[nodiscard]] HYLUX_FORCEINLINE float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] HYLUX_FORCEINLINE Vec3 Normalized() const noexcept
    {
        const float len = Length();
        if (len <= kEpsilon)
        {
            return Vec3{SimdZero()};
        }
        return Vec3{SimdMul(v_, SimdSplat(1.0f / len))};
    }

    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 HYLUX_VECTORCALL Min(Vec3 a, Vec3 b) noexcept
    {
        return Vec3{SimdMin(a.v_, b.v_)};
    }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 HYLUX_VECTORCALL Max(Vec3 a, Vec3 b) noexcept
    {
        return Vec3{SimdMax(a.v_, b.v_)};
    }

    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 HYLUX_VECTORCALL Lerp(Vec3 a, Vec3 b, float t) noexcept
    {
        return Vec3{SimdMulAdd(SimdSub(b.v_, a.v_), SimdSplat(t), a.v_)};
    }

    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 Zero() noexcept { return Vec3{0.0f, 0.0f, 0.0f}; }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 One() noexcept { return Vec3{1.0f, 1.0f, 1.0f}; }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 UnitX() noexcept { return Vec3{1.0f, 0.0f, 0.0f}; }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 UnitY() noexcept { return Vec3{0.0f, 1.0f, 0.0f}; }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec3 UnitZ() noexcept { return Vec3{0.0f, 0.0f, 1.0f}; }

private:
    SimdF32x4 v_;
};

[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator+(Vec3 a, Vec3 b) noexcept
{
    return Vec3{SimdAdd(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator-(Vec3 a, Vec3 b) noexcept
{
    return Vec3{SimdSub(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator*(Vec3 a, Vec3 b) noexcept
{
    return Vec3{SimdMul(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator*(Vec3 a, float s) noexcept
{
    return Vec3{SimdMul(a.Raw(), SimdSplat(s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator*(float s, Vec3 a) noexcept
{
    return Vec3{SimdMul(a.Raw(), SimdSplat(s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator/(Vec3 a, float s) noexcept
{
    return Vec3{SimdMul(a.Raw(), SimdSplat(1.0f / s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator-(Vec3 a) noexcept
{
    return Vec3{SimdNeg(a.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE bool HYLUX_VECTORCALL operator==(Vec3 a, Vec3 b) noexcept
{
    // Compare only the first three lanes (W lane could be polluted by upstream ops).
    return SimdGetX(a.Raw()) == SimdGetX(b.Raw()) && SimdGetY(a.Raw()) == SimdGetY(b.Raw()) &&
           SimdGetZ(a.Raw()) == SimdGetZ(b.Raw());
}
[[nodiscard]] HYLUX_FORCEINLINE bool HYLUX_VECTORCALL operator!=(Vec3 a, Vec3 b) noexcept
{
    return !(a == b);
}

static_assert(sizeof(Vec3) == 16, "Vec3 must be 16 bytes");
static_assert(alignof(Vec3) == 16, "Vec3 must be 16-byte aligned");
static_assert(sizeof(Vec3Packed) == 12, "Vec3Packed must be 12 bytes");

} // namespace Hylux::Math
