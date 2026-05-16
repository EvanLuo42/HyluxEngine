/// @file
/// @brief Out-of-line implementation for quaternion operations.

#include "Core/Math/Quat.h"

#include "Core/Math/Mat3.h"
#include "Core/Math/Mat4.h"

namespace Hylux::Math
{

Quat Quat::FromAxisAngle(Vec3 axis, float angleRadians) noexcept
{
    const float h = angleRadians * 0.5f;
    const float sinH = std::sin(h);
    const float cosH = std::cos(h);
    return Quat{axis.X() * sinH, axis.Y() * sinH, axis.Z() * sinH, cosH};
}

Quat Quat::FromEuler(float pitchX, float yawY, float rollZ) noexcept
{
    const float hp = pitchX * 0.5f;
    const float hy = yawY * 0.5f;
    const float hr = rollZ * 0.5f;
    const float cx = std::cos(hp), sx = std::sin(hp);
    const float cy = std::cos(hy), sy = std::sin(hy);
    const float cz = std::cos(hr), sz = std::sin(hr);

    // Y * X * Z composition (yaw, then pitch, then roll), unit output.
    return Quat{cy * sx * cz + sy * cx * sz, sy * cx * cz - cy * sx * sz, cy * cx * sz - sy * sx * cz,
                cy * cx * cz + sy * sx * sz};
}

Quat Quat::Slerp(Quat a, Quat b, float t) noexcept
{
    float cosTheta = SimdGetX(SimdDot4(a.Raw(), b.Raw()));

    SimdF32x4 bRaw = b.Raw();
    if (cosTheta < 0.0f)
    {
        bRaw = SimdNeg(bRaw);
        cosTheta = -cosTheta;
    }

    // Near-parallel: fall back to linear interpolation + renormalize.
    if (cosTheta > 1.0f - kEpsilon)
    {
        const SimdF32x4 r = SimdMulAdd(SimdSub(bRaw, a.Raw()), SimdSplat(t), a.Raw());
        return Quat{r}.Normalized();
    }

    const float theta = std::acos(cosTheta);
    const float sinTheta = std::sin(theta);
    const float wa = std::sin((1.0f - t) * theta) / sinTheta;
    const float wb = std::sin(t * theta) / sinTheta;
    return Quat{SimdAdd(SimdMul(a.Raw(), SimdSplat(wa)), SimdMul(bRaw, SimdSplat(wb)))};
}

Vec3 Quat::Rotate(Vec3 v) const noexcept
{
    // v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
    const Vec3 q{X(), Y(), Z()};
    const Vec3 t = Vec3::Cross(q, v) * 2.0f;
    return v + t * W() + Vec3::Cross(q, t);
}

Mat3 Quat::ToMat3() const noexcept
{
    const float x = X(), y = Y(), z = Z(), w = W();
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    const SimdF32x4 c0 = SimdSet(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
    const SimdF32x4 c1 = SimdSet(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
    const SimdF32x4 c2 = SimdSet(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
    return Mat3::FromColumnsRaw(c0, c1, c2);
}

Mat4 Quat::ToMat4() const noexcept
{
    const float x = X(), y = Y(), z = Z(), w = W();
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    const SimdF32x4 c0 = SimdSet(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
    const SimdF32x4 c1 = SimdSet(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
    const SimdF32x4 c2 = SimdSet(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
    const SimdF32x4 c3 = SimdSet(0.0f, 0.0f, 0.0f, 1.0f);
    return Mat4::FromColumnsRaw(c0, c1, c2, c3);
}

Quat operator*(Quat a, Quat b) noexcept
{
    const float ax = a.X(), ay = a.Y(), az = a.Z(), aw = a.W();
    const float bx = b.X(), by = b.Y(), bz = b.Z(), bw = b.W();
    return Quat{aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz};
}

} // namespace Hylux::Math
