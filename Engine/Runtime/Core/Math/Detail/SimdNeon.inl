/// @file
/// @brief ARM NEON (AArch64) implementation of the L0 SIMD API.

#pragma once

#include "Core/Math/Detail/PlatformDefines.h"

#include <arm_neon.h>
#include <cstdint>

namespace Hylux::Math
{

struct HYLUX_ALIGN(16) SimdF32x4
{
    float32x4_t v_;
};

struct HYLUX_ALIGN(16) SimdI32x4
{
    int32x4_t v_;
};

struct HYLUX_ALIGN(16) SimdMask32x4
{
    uint32x4_t v_;
};

// ----- Construction / conversion ------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadAligned(const float* src) noexcept
{
    return SimdF32x4{vld1q_f32(src)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadUnaligned(const float* src) noexcept
{
    return SimdF32x4{vld1q_f32(src)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSet(float x, float y, float z, float w) noexcept
{
    alignas(16) const float lit[4] = {x, y, z, w};
    return SimdF32x4{vld1q_f32(lit)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSplat(float v) noexcept
{
    return SimdF32x4{vdupq_n_f32(v)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdZero() noexcept
{
    return SimdF32x4{vdupq_n_f32(0.0f)};
}

HYLUX_FORCEINLINE void SimdStoreAligned(float* dst, SimdF32x4 v) noexcept
{
    vst1q_f32(dst, v.v_);
}

HYLUX_FORCEINLINE void SimdStoreUnaligned(float* dst, SimdF32x4 v) noexcept
{
    vst1q_f32(dst, v.v_);
}

[[nodiscard]] HYLUX_FORCEINLINE float SimdGetX(SimdF32x4 v) noexcept
{
    return vgetq_lane_f32(v.v_, 0);
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetY(SimdF32x4 v) noexcept
{
    return vgetq_lane_f32(v.v_, 1);
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetZ(SimdF32x4 v) noexcept
{
    return vgetq_lane_f32(v.v_, 2);
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetW(SimdF32x4 v) noexcept
{
    return vgetq_lane_f32(v.v_, 3);
}

// ----- Arithmetic ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAdd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vaddq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSub(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vsubq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMul(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vmulq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDiv(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vdivq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdNeg(SimdF32x4 a) noexcept
{
    return SimdF32x4{vnegq_f32(a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMin(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vminq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMax(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vmaxq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMulAdd(SimdF32x4 a, SimdF32x4 b, SimdF32x4 c) noexcept
{
    return SimdF32x4{vfmaq_f32(c.v_, a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRcp(SimdF32x4 a) noexcept
{
    return SimdF32x4{vdivq_f32(vdupq_n_f32(1.0f), a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{vsqrtq_f32(a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRsqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(a.v_))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAbs(SimdF32x4 a) noexcept
{
    return SimdF32x4{vabsq_f32(a.v_)};
}

// ----- Compare ------------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpEq(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{vceqq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{vcltq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{vcleq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{vcgtq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{vcgeq_f32(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAllTrue(SimdMask32x4 m) noexcept
{
    return vminvq_u32(m.v_) == 0xFFFFFFFFu;
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAnyTrue(SimdMask32x4 m) noexcept
{
    return vmaxvq_u32(m.v_) != 0u;
}

// ----- Logical / select ---------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAnd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a.v_), vreinterpretq_u32_f32(b.v_)))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdOr(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a.v_), vreinterpretq_u32_f32(b.v_)))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdXor(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a.v_), vreinterpretq_u32_f32(b.v_)))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAndNot(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(b.v_), vreinterpretq_u32_f32(a.v_)))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSelect(SimdMask32x4 mask, SimdF32x4 ifTrue, SimdF32x4 ifFalse) noexcept
{
    return SimdF32x4{vbslq_f32(mask.v_, ifTrue.v_, ifFalse.v_)};
}

// ----- Shuffle ------------------------------------------------------------------

template<int I0, int I1, int I2, int I3> [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle(SimdF32x4 v) noexcept
{
    static_assert(I0 >= 0 && I0 < 4 && I1 >= 0 && I1 < 4 && I2 >= 0 && I2 < 4 && I3 >= 0 && I3 < 4,
                  "SimdShuffle lane indices must be in [0, 4)");
    float32x4_t out = v.v_;
    out = vsetq_lane_f32(vgetq_lane_f32(v.v_, I0), out, 0);
    out = vsetq_lane_f32(vgetq_lane_f32(v.v_, I1), out, 1);
    out = vsetq_lane_f32(vgetq_lane_f32(v.v_, I2), out, 2);
    out = vsetq_lane_f32(vgetq_lane_f32(v.v_, I3), out, 3);
    return SimdF32x4{out};
}

template<int A0, int A1, int B0, int B1>
[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle2(SimdF32x4 a, SimdF32x4 b) noexcept
{
    static_assert(A0 >= 0 && A0 < 4 && A1 >= 0 && A1 < 4 && B0 >= 0 && B0 < 4 && B1 >= 0 && B1 < 4,
                  "SimdShuffle2 lane indices must be in [0, 4)");
    float32x4_t out = vdupq_n_f32(0.0f);
    out = vsetq_lane_f32(vgetq_lane_f32(a.v_, A0), out, 0);
    out = vsetq_lane_f32(vgetq_lane_f32(a.v_, A1), out, 1);
    out = vsetq_lane_f32(vgetq_lane_f32(b.v_, B0), out, 2);
    out = vsetq_lane_f32(vgetq_lane_f32(b.v_, B1), out, 3);
    return SimdF32x4{out};
}

// ----- Horizontal ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot4(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const float32x4_t p = vmulq_f32(a.v_, b.v_);
    return SimdF32x4{vdupq_n_f32(vaddvq_f32(p))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const float32x4_t p = vmulq_f32(a.v_, b.v_);
    const float d = vgetq_lane_f32(p, 0) + vgetq_lane_f32(p, 1) + vgetq_lane_f32(p, 2);
    return SimdF32x4{vdupq_n_f32(d)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdCross3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const float ax = vgetq_lane_f32(a.v_, 0);
    const float ay = vgetq_lane_f32(a.v_, 1);
    const float az = vgetq_lane_f32(a.v_, 2);
    const float bx = vgetq_lane_f32(b.v_, 0);
    const float by = vgetq_lane_f32(b.v_, 1);
    const float bz = vgetq_lane_f32(b.v_, 2);
    alignas(16) const float lit[4] = {ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx, 0.0f};
    return SimdF32x4{vld1q_f32(lit)};
}

} // namespace Hylux::Math
