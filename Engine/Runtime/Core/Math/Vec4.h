/// @file
/// @brief 4-component float vector backed by the L0 SIMD register.

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Simd.h"

namespace Hylux::Math
{

/// @brief 4-component float vector. 16-byte aligned, SIMD-native storage.
class HYLUX_ALIGN(16) Vec4
{
public:
    HYLUX_FORCEINLINE Vec4() noexcept : v_(SimdZero()) {}
    HYLUX_FORCEINLINE Vec4(float x, float y, float z, float w) noexcept : v_(SimdSet(x, y, z, w)) {}
    HYLUX_FORCEINLINE explicit Vec4(float splat) noexcept : v_(SimdSplat(splat)) {}
    HYLUX_FORCEINLINE explicit Vec4(SimdF32x4 raw) noexcept : v_(raw) {}

    /// @brief Loads four contiguous floats from a 16-byte aligned address.
    [[nodiscard]] HYLUX_FORCEINLINE static Vec4 LoadAligned(const float* src) noexcept
    {
        return Vec4{SimdLoadAligned(src)};
    }

    /// @brief Loads four contiguous floats from an unaligned address.
    [[nodiscard]] HYLUX_FORCEINLINE static Vec4 LoadUnaligned(const float* src) noexcept
    {
        return Vec4{SimdLoadUnaligned(src)};
    }

    HYLUX_FORCEINLINE void StoreAligned(float* dst) const noexcept { SimdStoreAligned(dst, v_); }
    HYLUX_FORCEINLINE void StoreUnaligned(float* dst) const noexcept { SimdStoreUnaligned(dst, v_); }

    [[nodiscard]] HYLUX_FORCEINLINE float X() const noexcept { return SimdGetX(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Y() const noexcept { return SimdGetY(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float Z() const noexcept { return SimdGetZ(v_); }
    [[nodiscard]] HYLUX_FORCEINLINE float W() const noexcept { return SimdGetW(v_); }

    /// @brief Returns the underlying SIMD register (escape hatch for L0 ops).
    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 Raw() const noexcept { return v_; }

    /// @brief Dot product (returns broadcast scalar in all lanes).
    [[nodiscard]] HYLUX_FORCEINLINE static float HYLUX_VECTORCALL Dot(Vec4 a, Vec4 b) noexcept
    {
        return SimdGetX(SimdDot4(a.v_, b.v_));
    }

    /// @brief Squared L2 length.
    [[nodiscard]] HYLUX_FORCEINLINE float LengthSquared() const noexcept { return SimdGetX(SimdDot4(v_, v_)); }

    /// @brief L2 length.
    [[nodiscard]] HYLUX_FORCEINLINE float Length() const noexcept { return std::sqrt(LengthSquared()); }

    /// @brief Returns a unit-length copy. Returns the zero vector if length is below kEpsilon.
    [[nodiscard]] HYLUX_FORCEINLINE Vec4 Normalized() const noexcept
    {
        const SimdF32x4 d2 = SimdDot4(v_, v_);
        const float len = std::sqrt(SimdGetX(d2));
        if (len <= kEpsilon)
        {
            return Vec4{SimdZero()};
        }
        return Vec4{SimdMul(v_, SimdSplat(1.0f / len))};
    }

    /// @brief Component-wise min / max.
    [[nodiscard]] HYLUX_FORCEINLINE static Vec4 HYLUX_VECTORCALL Min(Vec4 a, Vec4 b) noexcept
    {
        return Vec4{SimdMin(a.v_, b.v_)};
    }
    [[nodiscard]] HYLUX_FORCEINLINE static Vec4 HYLUX_VECTORCALL Max(Vec4 a, Vec4 b) noexcept
    {
        return Vec4{SimdMax(a.v_, b.v_)};
    }

    /// @brief Linear interpolation from a to b by t (unclamped).
    [[nodiscard]] HYLUX_FORCEINLINE static Vec4 HYLUX_VECTORCALL Lerp(Vec4 a, Vec4 b, float t) noexcept
    {
        return Vec4{SimdMulAdd(SimdSub(b.v_, a.v_), SimdSplat(t), a.v_)};
    }

private:
    SimdF32x4 v_;
};

[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator+(Vec4 a, Vec4 b) noexcept
{
    return Vec4{SimdAdd(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator-(Vec4 a, Vec4 b) noexcept
{
    return Vec4{SimdSub(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator*(Vec4 a, Vec4 b) noexcept
{
    return Vec4{SimdMul(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator/(Vec4 a, Vec4 b) noexcept
{
    return Vec4{SimdDiv(a.Raw(), b.Raw())};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator*(Vec4 a, float s) noexcept
{
    return Vec4{SimdMul(a.Raw(), SimdSplat(s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator*(float s, Vec4 a) noexcept
{
    return Vec4{SimdMul(a.Raw(), SimdSplat(s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator/(Vec4 a, float s) noexcept
{
    return Vec4{SimdMul(a.Raw(), SimdSplat(1.0f / s))};
}
[[nodiscard]] HYLUX_FORCEINLINE Vec4 HYLUX_VECTORCALL operator-(Vec4 a) noexcept
{
    return Vec4{SimdNeg(a.Raw())};
}

[[nodiscard]] HYLUX_FORCEINLINE bool HYLUX_VECTORCALL operator==(Vec4 a, Vec4 b) noexcept
{
    return SimdAllTrue(SimdCmpEq(a.Raw(), b.Raw()));
}
[[nodiscard]] HYLUX_FORCEINLINE bool HYLUX_VECTORCALL operator!=(Vec4 a, Vec4 b) noexcept
{
    return !(a == b);
}

static_assert(sizeof(Vec4) == 16, "Vec4 must be 16 bytes");
static_assert(alignof(Vec4) == 16, "Vec4 must be 16-byte aligned");

} // namespace Hylux::Math
