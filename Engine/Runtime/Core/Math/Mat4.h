/// @file
/// @brief 4x4 float matrix. Column-major storage, column-vector convention
///        (graphics-standard: `M * v` is "apply M to v"). Matches GLM /
///        Unity / GLSL default layout, so a Mat4 can be uploaded directly
///        to a uniform buffer as four contiguous float4 columns.

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Simd.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

namespace Hylux::Math
{

/// @brief 4x4 matrix in column-major storage. Identity by default.
class HYLUX_ALIGN(16) Mat4
{
public:
    HYLUX_FORCEINLINE Mat4() noexcept
    {
        cols_[0] = SimdSet(1.0f, 0.0f, 0.0f, 0.0f);
        cols_[1] = SimdSet(0.0f, 1.0f, 0.0f, 0.0f);
        cols_[2] = SimdSet(0.0f, 0.0f, 1.0f, 0.0f);
        cols_[3] = SimdSet(0.0f, 0.0f, 0.0f, 1.0f);
    }

    HYLUX_FORCEINLINE Mat4(Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3) noexcept
    {
        cols_[0] = c0.Raw();
        cols_[1] = c1.Raw();
        cols_[2] = c2.Raw();
        cols_[3] = c3.Raw();
    }

    /// @brief Returns column j as a Vec4.
    [[nodiscard]] HYLUX_FORCEINLINE Vec4 Column(int j) const noexcept { return Vec4{cols_[j]}; }

    /// @brief Returns the raw SIMD column j (escape hatch for L0 ops).
    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 ColumnRaw(int j) const noexcept { return cols_[j]; }

    /// @brief Builds an identity matrix.
    [[nodiscard]] static HYLUX_FORCEINLINE Mat4 Identity() noexcept { return Mat4{}; }

    /// @brief Builds a uniform scale matrix.
    [[nodiscard]] static HYLUX_FORCEINLINE Mat4 Scale(Vec3 s) noexcept
    {
        Mat4 m;
        m.cols_[0] = SimdSet(s.X(), 0.0f, 0.0f, 0.0f);
        m.cols_[1] = SimdSet(0.0f, s.Y(), 0.0f, 0.0f);
        m.cols_[2] = SimdSet(0.0f, 0.0f, s.Z(), 0.0f);
        m.cols_[3] = SimdSet(0.0f, 0.0f, 0.0f, 1.0f);
        return m;
    }

    /// @brief Builds a translation matrix.
    [[nodiscard]] static HYLUX_FORCEINLINE Mat4 Translation(Vec3 t) noexcept
    {
        Mat4 m;
        m.cols_[3] = SimdSet(t.X(), t.Y(), t.Z(), 1.0f);
        return m;
    }

    /// @brief Builds a right-handed look-at matrix (camera at eye looking toward target with up).
    [[nodiscard]] static Mat4 LookAtRH(Vec3 eye, Vec3 target, Vec3 up) noexcept;

    /// @brief Builds a right-handed perspective projection that maps depth to [0, 1]
    ///        (D3D / Vulkan / Metal convention).
    [[nodiscard]] static Mat4 PerspectiveRH_ZO(float fovYRadians, float aspect, float nearZ, float farZ) noexcept;

    /// @brief Builds a right-handed orthographic projection mapping depth to [0, 1].
    [[nodiscard]] static Mat4 OrthoRH_ZO(float left, float right, float bottom, float top, float nearZ,
                                         float farZ) noexcept;

    /// @brief Transpose.
    [[nodiscard]] Mat4 Transposed() const noexcept;

    /// @brief Full general 4x4 inverse. Falls back to identity if singular.
    [[nodiscard]] Mat4 Inverse() const noexcept;

    /// @brief Constructs from four raw SIMD columns. Reserved for math-internal use.
    [[nodiscard]] static HYLUX_FORCEINLINE Mat4 FromColumnsRaw(SimdF32x4 c0, SimdF32x4 c1, SimdF32x4 c2,
                                                               SimdF32x4 c3) noexcept
    {
        Mat4 m;
        m.cols_[0] = c0;
        m.cols_[1] = c1;
        m.cols_[2] = c2;
        m.cols_[3] = c3;
        return m;
    }

private:
    SimdF32x4 cols_[4];

    friend Mat4 operator*(const Mat4& a, const Mat4& b) noexcept;
};

[[nodiscard]] HYLUX_FORCEINLINE Vec4 operator*(const Mat4& m, Vec4 v) noexcept
{
    // Column-major M * v = sum_j v[j] * cols[j]
    const SimdF32x4 r =
        SimdMulAdd(m.ColumnRaw(0), SimdSplat(v.X()),
                   SimdMulAdd(m.ColumnRaw(1), SimdSplat(v.Y()),
                              SimdMulAdd(m.ColumnRaw(2), SimdSplat(v.Z()), SimdMul(m.ColumnRaw(3), SimdSplat(v.W())))));
    return Vec4{r};
}

[[nodiscard]] Mat4 operator*(const Mat4& a, const Mat4& b) noexcept;

static_assert(sizeof(Mat4) == 64, "Mat4 must be 64 bytes");
static_assert(alignof(Mat4) == 16, "Mat4 must be 16-byte aligned");

} // namespace Hylux::Math
