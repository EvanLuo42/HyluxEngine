/// @file
/// @brief 3x3 float matrix. Stored as three SIMD columns padded to 16 bytes
///        (W lane = 0). Column-major / column-vector convention, matching
///        Mat4. Primarily used for rotation / normal matrices.

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Simd.h"
#include "Core/Math/Vec3.h"

namespace Hylux::Math
{

class HYLUX_ALIGN(16) Mat3
{
public:
    HYLUX_FORCEINLINE Mat3() noexcept
    {
        cols_[0] = SimdSet(1.0f, 0.0f, 0.0f, 0.0f);
        cols_[1] = SimdSet(0.0f, 1.0f, 0.0f, 0.0f);
        cols_[2] = SimdSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    HYLUX_FORCEINLINE Mat3(Vec3 c0, Vec3 c1, Vec3 c2) noexcept
    {
        cols_[0] = c0.Raw();
        cols_[1] = c1.Raw();
        cols_[2] = c2.Raw();
    }

    [[nodiscard]] HYLUX_FORCEINLINE Vec3 Column(int j) const noexcept { return Vec3{cols_[j]}; }
    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 ColumnRaw(int j) const noexcept { return cols_[j]; }

    [[nodiscard]] static HYLUX_FORCEINLINE Mat3 Identity() noexcept { return Mat3{}; }

    [[nodiscard]] Mat3 Transposed() const noexcept;

    [[nodiscard]] static HYLUX_FORCEINLINE Mat3 FromColumnsRaw(SimdF32x4 c0, SimdF32x4 c1, SimdF32x4 c2) noexcept
    {
        Mat3 m;
        m.cols_[0] = c0;
        m.cols_[1] = c1;
        m.cols_[2] = c2;
        return m;
    }

private:
    SimdF32x4 cols_[3];

    friend Mat3 operator*(const Mat3& a, const Mat3& b) noexcept;
};

[[nodiscard]] HYLUX_FORCEINLINE Vec3 operator*(const Mat3& m, Vec3 v) noexcept
{
    const SimdF32x4 r =
        SimdMulAdd(m.ColumnRaw(0), SimdSplat(v.X()),
                   SimdMulAdd(m.ColumnRaw(1), SimdSplat(v.Y()), SimdMul(m.ColumnRaw(2), SimdSplat(v.Z()))));
    return Vec3{r};
}

[[nodiscard]] Mat3 operator*(const Mat3& a, const Mat3& b) noexcept;

static_assert(sizeof(Mat3) == 48, "Mat3 must be 48 bytes (3 x 16)");
static_assert(alignof(Mat3) == 16, "Mat3 must be 16-byte aligned");

} // namespace Hylux::Math
