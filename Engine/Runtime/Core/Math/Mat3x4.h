/// @file
/// @brief 3x4 affine matrix, row-major storage (3 SimdF32x4 rows). Lays out identical to
///        HLSL `float3x4` / GLSL `mat4x3` when uploaded to a structured buffer, so the
///        renderer can `memcpy` straight from this struct into TransformDoubleBuffer
///        slots without a CPU-side conversion. Encodes a 3D affine transform: the top 3
///        rows of a full 4x4 column-vector matrix (the implicit bottom row is (0,0,0,1)).

#pragma once

#include "Core/Math/Common.h"
#include "Core/Math/Mat4.h"
#include "Core/Math/Simd.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

#include <cstddef>

namespace Hylux::Math
{

/// @brief Affine 3x4 matrix. Each row is one output component (x, y, z); columns 0..2
///        hold the rotation/scale basis, column 3 holds the translation.
class HYLUX_ALIGN(16) Mat3x4
{
public:
    HYLUX_FORCEINLINE Mat3x4() noexcept
    {
        rows_[0] = SimdSet(1.0f, 0.0f, 0.0f, 0.0f);
        rows_[1] = SimdSet(0.0f, 1.0f, 0.0f, 0.0f);
        rows_[2] = SimdSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    HYLUX_FORCEINLINE Mat3x4(Vec4 r0, Vec4 r1, Vec4 r2) noexcept
    {
        rows_[0] = r0.Raw();
        rows_[1] = r1.Raw();
        rows_[2] = r2.Raw();
    }

    [[nodiscard]] static HYLUX_FORCEINLINE Mat3x4 Identity() noexcept { return Mat3x4{}; }

    /// @brief Extracts the top-3 rows of the supplied column-major Mat4. The Mat4's bottom
    ///        row is assumed to be (0, 0, 0, 1); affine input only.
    [[nodiscard]] static Mat3x4 FromMat4(const Mat4& m4) noexcept;

    /// @brief Constructs from raw row registers. Reserved for math-internal callers.
    [[nodiscard]] static HYLUX_FORCEINLINE Mat3x4 FromRowsRaw(SimdF32x4 r0,
                                                              SimdF32x4 r1,
                                                              SimdF32x4 r2) noexcept
    {
        Mat3x4 m;
        m.rows_[0] = r0;
        m.rows_[1] = r1;
        m.rows_[2] = r2;
        return m;
    }

    [[nodiscard]] HYLUX_FORCEINLINE Vec4 Row(int i) const noexcept { return Vec4{rows_[i]}; }
    [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 RowRaw(int i) const noexcept { return rows_[i]; }

    /// @brief Applies the matrix to a point (implicit w = 1).
    [[nodiscard]] HYLUX_FORCEINLINE Vec3 TransformPoint(Vec3 p) const noexcept
    {
        const SimdF32x4 p4 = SimdSet(p.X(), p.Y(), p.Z(), 1.0f);
        return Vec3{SimdGetX(SimdDot4(rows_[0], p4)),
                    SimdGetX(SimdDot4(rows_[1], p4)),
                    SimdGetX(SimdDot4(rows_[2], p4))};
    }

    /// @brief Applies the matrix to a direction (implicit w = 0; translation column ignored).
    [[nodiscard]] HYLUX_FORCEINLINE Vec3 TransformDir(Vec3 d) const noexcept
    {
        const SimdF32x4 d4 = SimdSet(d.X(), d.Y(), d.Z(), 0.0f);
        return Vec3{SimdGetX(SimdDot4(rows_[0], d4)),
                    SimdGetX(SimdDot4(rows_[1], d4)),
                    SimdGetX(SimdDot4(rows_[2], d4))};
    }

    /// @brief Writes the 12-float row-major payload that maps to HLSL `float3x4`. The
    ///        destination must be 16-byte aligned; 48 bytes are written.
    HYLUX_FORCEINLINE void StoreRowMajorAligned(float* dst) const noexcept
    {
        SimdStoreAligned(dst + 0,  rows_[0]);
        SimdStoreAligned(dst + 4,  rows_[1]);
        SimdStoreAligned(dst + 8,  rows_[2]);
    }

    /// @brief Unaligned variant of StoreRowMajorAligned.
    HYLUX_FORCEINLINE void StoreRowMajor(float* dst) const noexcept
    {
        SimdStoreUnaligned(dst + 0,  rows_[0]);
        SimdStoreUnaligned(dst + 4,  rows_[1]);
        SimdStoreUnaligned(dst + 8,  rows_[2]);
    }

private:
    SimdF32x4 rows_[3];
};

[[nodiscard]] HYLUX_FORCEINLINE Vec3 HYLUX_VECTORCALL operator*(const Mat3x4& m, Vec3 p) noexcept
{
    return m.TransformPoint(p);
}

static_assert(sizeof(Mat3x4) == 48, "Mat3x4 must be 48 bytes (3 x 16)");
static_assert(alignof(Mat3x4) == 16, "Mat3x4 must be 16-byte aligned");

} // namespace Hylux::Math
