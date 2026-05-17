/// @file
/// @brief Out-of-line implementation for non-trivial Mat4 operations.

#include "Core/Math/Mat4.h"

namespace Hylux::Math
{

Mat4 operator*(const Mat4& a, const Mat4& b) noexcept
{
    auto composeColumn = [&](SimdF32x4 bj) {
        return SimdMulAdd(
            a.cols_[0], SimdSplat(SimdGetX(bj)),
            SimdMulAdd(a.cols_[1], SimdSplat(SimdGetY(bj)),
                       SimdMulAdd(a.cols_[2], SimdSplat(SimdGetZ(bj)), SimdMul(a.cols_[3], SimdSplat(SimdGetW(bj))))));
    };

    Mat4 r;
    r.cols_[0] = composeColumn(b.cols_[0]);
    r.cols_[1] = composeColumn(b.cols_[1]);
    r.cols_[2] = composeColumn(b.cols_[2]);
    r.cols_[3] = composeColumn(b.cols_[3]);
    return r;
}

Mat4 Mat4::Transposed() const noexcept
{
    // Read 16 floats, rebuild as transposed columns.
    alignas(16) float buf[16];
    SimdStoreAligned(buf + 0, cols_[0]);
    SimdStoreAligned(buf + 4, cols_[1]);
    SimdStoreAligned(buf + 8, cols_[2]);
    SimdStoreAligned(buf + 12, cols_[3]);

    Mat4 r;
    r.cols_[0] = SimdSet(buf[0], buf[4], buf[8], buf[12]);
    r.cols_[1] = SimdSet(buf[1], buf[5], buf[9], buf[13]);
    r.cols_[2] = SimdSet(buf[2], buf[6], buf[10], buf[14]);
    r.cols_[3] = SimdSet(buf[3], buf[7], buf[11], buf[15]);
    return r;
}

Mat4 Mat4::Inverse() const noexcept
{
    // Scalar cofactor expansion. SIMD inverse is possible but the gain over an
    // already-rare operation rarely justifies the implementation complexity.
    alignas(16) float m[16];
    SimdStoreAligned(m + 0, cols_[0]);
    SimdStoreAligned(m + 4, cols_[1]);
    SimdStoreAligned(m + 8, cols_[2]);
    SimdStoreAligned(m + 12, cols_[3]);

    // Column-major: m[col * 4 + row]. Build inv as a 4x4 float buffer in
    // row-major scratch then re-pack to column-major SimdF32x4.
    auto idx = [](int row, int col) { return col * 4 + row; };

    float inv[16];

    inv[0] = m[idx(1, 1)] * m[idx(2, 2)] * m[idx(3, 3)] - m[idx(1, 1)] * m[idx(2, 3)] * m[idx(3, 2)] -
             m[idx(2, 1)] * m[idx(1, 2)] * m[idx(3, 3)] + m[idx(2, 1)] * m[idx(1, 3)] * m[idx(3, 2)] +
             m[idx(3, 1)] * m[idx(1, 2)] * m[idx(2, 3)] - m[idx(3, 1)] * m[idx(1, 3)] * m[idx(2, 2)];

    inv[4] = -m[idx(1, 0)] * m[idx(2, 2)] * m[idx(3, 3)] + m[idx(1, 0)] * m[idx(2, 3)] * m[idx(3, 2)] +
             m[idx(2, 0)] * m[idx(1, 2)] * m[idx(3, 3)] - m[idx(2, 0)] * m[idx(1, 3)] * m[idx(3, 2)] -
             m[idx(3, 0)] * m[idx(1, 2)] * m[idx(2, 3)] + m[idx(3, 0)] * m[idx(1, 3)] * m[idx(2, 2)];

    inv[8] = m[idx(1, 0)] * m[idx(2, 1)] * m[idx(3, 3)] - m[idx(1, 0)] * m[idx(2, 3)] * m[idx(3, 1)] -
             m[idx(2, 0)] * m[idx(1, 1)] * m[idx(3, 3)] + m[idx(2, 0)] * m[idx(1, 3)] * m[idx(3, 1)] +
             m[idx(3, 0)] * m[idx(1, 1)] * m[idx(2, 3)] - m[idx(3, 0)] * m[idx(1, 3)] * m[idx(2, 1)];

    inv[12] = -m[idx(1, 0)] * m[idx(2, 1)] * m[idx(3, 2)] + m[idx(1, 0)] * m[idx(2, 2)] * m[idx(3, 1)] +
              m[idx(2, 0)] * m[idx(1, 1)] * m[idx(3, 2)] - m[idx(2, 0)] * m[idx(1, 2)] * m[idx(3, 1)] -
              m[idx(3, 0)] * m[idx(1, 1)] * m[idx(2, 2)] + m[idx(3, 0)] * m[idx(1, 2)] * m[idx(2, 1)];

    inv[1] = -m[idx(0, 1)] * m[idx(2, 2)] * m[idx(3, 3)] + m[idx(0, 1)] * m[idx(2, 3)] * m[idx(3, 2)] +
             m[idx(2, 1)] * m[idx(0, 2)] * m[idx(3, 3)] - m[idx(2, 1)] * m[idx(0, 3)] * m[idx(3, 2)] -
             m[idx(3, 1)] * m[idx(0, 2)] * m[idx(2, 3)] + m[idx(3, 1)] * m[idx(0, 3)] * m[idx(2, 2)];

    inv[5] = m[idx(0, 0)] * m[idx(2, 2)] * m[idx(3, 3)] - m[idx(0, 0)] * m[idx(2, 3)] * m[idx(3, 2)] -
             m[idx(2, 0)] * m[idx(0, 2)] * m[idx(3, 3)] + m[idx(2, 0)] * m[idx(0, 3)] * m[idx(3, 2)] +
             m[idx(3, 0)] * m[idx(0, 2)] * m[idx(2, 3)] - m[idx(3, 0)] * m[idx(0, 3)] * m[idx(2, 2)];

    inv[9] = -m[idx(0, 0)] * m[idx(2, 1)] * m[idx(3, 3)] + m[idx(0, 0)] * m[idx(2, 3)] * m[idx(3, 1)] +
             m[idx(2, 0)] * m[idx(0, 1)] * m[idx(3, 3)] - m[idx(2, 0)] * m[idx(0, 3)] * m[idx(3, 1)] -
             m[idx(3, 0)] * m[idx(0, 1)] * m[idx(2, 3)] + m[idx(3, 0)] * m[idx(0, 3)] * m[idx(2, 1)];

    inv[13] = m[idx(0, 0)] * m[idx(2, 1)] * m[idx(3, 2)] - m[idx(0, 0)] * m[idx(2, 2)] * m[idx(3, 1)] -
              m[idx(2, 0)] * m[idx(0, 1)] * m[idx(3, 2)] + m[idx(2, 0)] * m[idx(0, 2)] * m[idx(3, 1)] +
              m[idx(3, 0)] * m[idx(0, 1)] * m[idx(2, 2)] - m[idx(3, 0)] * m[idx(0, 2)] * m[idx(2, 1)];

    inv[2] = m[idx(0, 1)] * m[idx(1, 2)] * m[idx(3, 3)] - m[idx(0, 1)] * m[idx(1, 3)] * m[idx(3, 2)] -
             m[idx(1, 1)] * m[idx(0, 2)] * m[idx(3, 3)] + m[idx(1, 1)] * m[idx(0, 3)] * m[idx(3, 2)] +
             m[idx(3, 1)] * m[idx(0, 2)] * m[idx(1, 3)] - m[idx(3, 1)] * m[idx(0, 3)] * m[idx(1, 2)];

    inv[6] = -m[idx(0, 0)] * m[idx(1, 2)] * m[idx(3, 3)] + m[idx(0, 0)] * m[idx(1, 3)] * m[idx(3, 2)] +
             m[idx(1, 0)] * m[idx(0, 2)] * m[idx(3, 3)] - m[idx(1, 0)] * m[idx(0, 3)] * m[idx(3, 2)] -
             m[idx(3, 0)] * m[idx(0, 2)] * m[idx(1, 3)] + m[idx(3, 0)] * m[idx(0, 3)] * m[idx(1, 2)];

    inv[10] = m[idx(0, 0)] * m[idx(1, 1)] * m[idx(3, 3)] - m[idx(0, 0)] * m[idx(1, 3)] * m[idx(3, 1)] -
              m[idx(1, 0)] * m[idx(0, 1)] * m[idx(3, 3)] + m[idx(1, 0)] * m[idx(0, 3)] * m[idx(3, 1)] +
              m[idx(3, 0)] * m[idx(0, 1)] * m[idx(1, 3)] - m[idx(3, 0)] * m[idx(0, 3)] * m[idx(1, 1)];

    inv[14] = -m[idx(0, 0)] * m[idx(1, 1)] * m[idx(3, 2)] + m[idx(0, 0)] * m[idx(1, 2)] * m[idx(3, 1)] +
              m[idx(1, 0)] * m[idx(0, 1)] * m[idx(3, 2)] - m[idx(1, 0)] * m[idx(0, 2)] * m[idx(3, 1)] -
              m[idx(3, 0)] * m[idx(0, 1)] * m[idx(1, 2)] + m[idx(3, 0)] * m[idx(0, 2)] * m[idx(1, 1)];

    inv[3] = -m[idx(0, 1)] * m[idx(1, 2)] * m[idx(2, 3)] + m[idx(0, 1)] * m[idx(1, 3)] * m[idx(2, 2)] +
             m[idx(1, 1)] * m[idx(0, 2)] * m[idx(2, 3)] - m[idx(1, 1)] * m[idx(0, 3)] * m[idx(2, 2)] -
             m[idx(2, 1)] * m[idx(0, 2)] * m[idx(1, 3)] + m[idx(2, 1)] * m[idx(0, 3)] * m[idx(1, 2)];

    inv[7] = m[idx(0, 0)] * m[idx(1, 2)] * m[idx(2, 3)] - m[idx(0, 0)] * m[idx(1, 3)] * m[idx(2, 2)] -
             m[idx(1, 0)] * m[idx(0, 2)] * m[idx(2, 3)] + m[idx(1, 0)] * m[idx(0, 3)] * m[idx(2, 2)] +
             m[idx(2, 0)] * m[idx(0, 2)] * m[idx(1, 3)] - m[idx(2, 0)] * m[idx(0, 3)] * m[idx(1, 2)];

    inv[11] = -m[idx(0, 0)] * m[idx(1, 1)] * m[idx(2, 3)] + m[idx(0, 0)] * m[idx(1, 3)] * m[idx(2, 1)] +
              m[idx(1, 0)] * m[idx(0, 1)] * m[idx(2, 3)] - m[idx(1, 0)] * m[idx(0, 3)] * m[idx(2, 1)] -
              m[idx(2, 0)] * m[idx(0, 1)] * m[idx(1, 3)] + m[idx(2, 0)] * m[idx(0, 3)] * m[idx(1, 1)];

    inv[15] = m[idx(0, 0)] * m[idx(1, 1)] * m[idx(2, 2)] - m[idx(0, 0)] * m[idx(1, 2)] * m[idx(2, 1)] -
              m[idx(1, 0)] * m[idx(0, 1)] * m[idx(2, 2)] + m[idx(1, 0)] * m[idx(0, 2)] * m[idx(2, 1)] +
              m[idx(2, 0)] * m[idx(0, 1)] * m[idx(1, 2)] - m[idx(2, 0)] * m[idx(0, 2)] * m[idx(1, 1)];

    float det = m[idx(0, 0)] * inv[0] + m[idx(0, 1)] * inv[4] + m[idx(0, 2)] * inv[8] + m[idx(0, 3)] * inv[12];
    if (IsNearlyZero(det))
    {
        return Mat4::Identity();
    }
    const float invDet = 1.0f / det;
    for (int i = 0; i < 16; ++i)
    {
        inv[i] *= invDet;
    }

    // Repack from the inv buffer (row-major in the formulas above) into column-major SimdF32x4 columns.
    Mat4 r;
    r.cols_[0] = SimdSet(inv[0], inv[4], inv[8],  inv[12]);
    r.cols_[1] = SimdSet(inv[1], inv[5], inv[9],  inv[13]);
    r.cols_[2] = SimdSet(inv[2], inv[6], inv[10], inv[14]);
    r.cols_[3] = SimdSet(inv[3], inv[7], inv[11], inv[15]);
    return r;
}

Mat4 Mat4::LookAtRH(Vec3 eye, Vec3 target, Vec3 up) noexcept
{
    const Vec3 f = (target - eye).Normalized();     // forward = target - eye, normalized
    const Vec3 s = Vec3::Cross(f, up).Normalized(); // right
    const Vec3 u = Vec3::Cross(s, f);               // recomputed up

    Mat4 m;
    m.cols_[0] = SimdSet(s.X(), u.X(), -f.X(), 0.0f);
    m.cols_[1] = SimdSet(s.Y(), u.Y(), -f.Y(), 0.0f);
    m.cols_[2] = SimdSet(s.Z(), u.Z(), -f.Z(), 0.0f);
    m.cols_[3] = SimdSet(-Vec3::Dot(s, eye), -Vec3::Dot(u, eye), Vec3::Dot(f, eye), 1.0f);
    return m;
}

Mat4 Mat4::PerspectiveRH_ZO(float fovYRadians, float aspect, float nearZ, float farZ) noexcept
{
    const float h = 1.0f / std::tan(fovYRadians * 0.5f);
    const float w = h / aspect;
    const float a = farZ / (nearZ - farZ);
    const float b = (nearZ * farZ) / (nearZ - farZ);

    Mat4 m;
    m.cols_[0] = SimdSet(w, 0.0f, 0.0f, 0.0f);
    m.cols_[1] = SimdSet(0.0f, h, 0.0f, 0.0f);
    m.cols_[2] = SimdSet(0.0f, 0.0f, a, -1.0f);
    m.cols_[3] = SimdSet(0.0f, 0.0f, b, 0.0f);
    return m;
}

Mat4 Mat4::OrthoRH_ZO(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept
{
    const float rw = 1.0f / (right - left);
    const float rh = 1.0f / (top - bottom);
    const float rd = 1.0f / (nearZ - farZ);

    Mat4 m;
    m.cols_[0] = SimdSet(2.0f * rw, 0.0f, 0.0f, 0.0f);
    m.cols_[1] = SimdSet(0.0f, 2.0f * rh, 0.0f, 0.0f);
    m.cols_[2] = SimdSet(0.0f, 0.0f, rd, 0.0f);
    m.cols_[3] = SimdSet(-(right + left) * rw, -(top + bottom) * rh, nearZ * rd, 1.0f);
    return m;
}

} // namespace Hylux::Math
