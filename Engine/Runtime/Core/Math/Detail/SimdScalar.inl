/// @file
/// @brief Portable scalar fallback implementation of the L0 SIMD API.

#pragma once

#include "Core/Math/Detail/PlatformDefines.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace Hylux::Math
{

struct HYLUX_ALIGN(16) SimdF32x4
{
    float v_[4];
};

struct HYLUX_ALIGN(16) SimdI32x4
{
    std::int32_t v_[4];
};

struct HYLUX_ALIGN(16) SimdMask32x4
{
    std::uint32_t v_[4];
};

// ----- Construction / conversion ------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadAligned(const float* src) noexcept
{
    return SimdF32x4{{src[0], src[1], src[2], src[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadUnaligned(const float* src) noexcept
{
    SimdF32x4 r;
    std::memcpy(r.v_, src, sizeof(float) * 4);
    return r;
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSet(float x, float y, float z, float w) noexcept
{
    return SimdF32x4{{x, y, z, w}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSplat(float v) noexcept
{
    return SimdF32x4{{v, v, v, v}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdZero() noexcept
{
    return SimdF32x4{{0.0f, 0.0f, 0.0f, 0.0f}};
}

HYLUX_FORCEINLINE void SimdStoreAligned(float* dst, SimdF32x4 v) noexcept
{
    dst[0] = v.v_[0];
    dst[1] = v.v_[1];
    dst[2] = v.v_[2];
    dst[3] = v.v_[3];
}

HYLUX_FORCEINLINE void SimdStoreUnaligned(float* dst, SimdF32x4 v) noexcept
{
    std::memcpy(dst, v.v_, sizeof(float) * 4);
}

[[nodiscard]] HYLUX_FORCEINLINE float SimdGetX(SimdF32x4 v) noexcept
{
    return v.v_[0];
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetY(SimdF32x4 v) noexcept
{
    return v.v_[1];
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetZ(SimdF32x4 v) noexcept
{
    return v.v_[2];
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetW(SimdF32x4 v) noexcept
{
    return v.v_[3];
}

// ----- Arithmetic ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAdd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] + b.v_[0], a.v_[1] + b.v_[1], a.v_[2] + b.v_[2], a.v_[3] + b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSub(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] - b.v_[0], a.v_[1] - b.v_[1], a.v_[2] - b.v_[2], a.v_[3] - b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMul(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] * b.v_[0], a.v_[1] * b.v_[1], a.v_[2] * b.v_[2], a.v_[3] * b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDiv(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] / b.v_[0], a.v_[1] / b.v_[1], a.v_[2] / b.v_[2], a.v_[3] / b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdNeg(SimdF32x4 a) noexcept
{
    return SimdF32x4{{-a.v_[0], -a.v_[1], -a.v_[2], -a.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMin(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] < b.v_[0] ? a.v_[0] : b.v_[0], a.v_[1] < b.v_[1] ? a.v_[1] : b.v_[1],
                      a.v_[2] < b.v_[2] ? a.v_[2] : b.v_[2], a.v_[3] < b.v_[3] ? a.v_[3] : b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMax(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[0] > b.v_[0] ? a.v_[0] : b.v_[0], a.v_[1] > b.v_[1] ? a.v_[1] : b.v_[1],
                      a.v_[2] > b.v_[2] ? a.v_[2] : b.v_[2], a.v_[3] > b.v_[3] ? a.v_[3] : b.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMulAdd(SimdF32x4 a, SimdF32x4 b, SimdF32x4 c) noexcept
{
    return SimdF32x4{{a.v_[0] * b.v_[0] + c.v_[0], a.v_[1] * b.v_[1] + c.v_[1], a.v_[2] * b.v_[2] + c.v_[2],
                      a.v_[3] * b.v_[3] + c.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRcp(SimdF32x4 a) noexcept
{
    return SimdF32x4{{1.0f / a.v_[0], 1.0f / a.v_[1], 1.0f / a.v_[2], 1.0f / a.v_[3]}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{{std::sqrt(a.v_[0]), std::sqrt(a.v_[1]), std::sqrt(a.v_[2]), std::sqrt(a.v_[3])}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRsqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{
        {1.0f / std::sqrt(a.v_[0]), 1.0f / std::sqrt(a.v_[1]), 1.0f / std::sqrt(a.v_[2]), 1.0f / std::sqrt(a.v_[3])}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAbs(SimdF32x4 a) noexcept
{
    return SimdF32x4{{std::fabs(a.v_[0]), std::fabs(a.v_[1]), std::fabs(a.v_[2]), std::fabs(a.v_[3])}};
}

// ----- Compare ------------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpEq(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{{a.v_[0] == b.v_[0] ? 0xFFFFFFFFu : 0u, a.v_[1] == b.v_[1] ? 0xFFFFFFFFu : 0u,
                         a.v_[2] == b.v_[2] ? 0xFFFFFFFFu : 0u, a.v_[3] == b.v_[3] ? 0xFFFFFFFFu : 0u}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{{a.v_[0] < b.v_[0] ? 0xFFFFFFFFu : 0u, a.v_[1] < b.v_[1] ? 0xFFFFFFFFu : 0u,
                         a.v_[2] < b.v_[2] ? 0xFFFFFFFFu : 0u, a.v_[3] < b.v_[3] ? 0xFFFFFFFFu : 0u}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{{a.v_[0] <= b.v_[0] ? 0xFFFFFFFFu : 0u, a.v_[1] <= b.v_[1] ? 0xFFFFFFFFu : 0u,
                         a.v_[2] <= b.v_[2] ? 0xFFFFFFFFu : 0u, a.v_[3] <= b.v_[3] ? 0xFFFFFFFFu : 0u}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{{a.v_[0] > b.v_[0] ? 0xFFFFFFFFu : 0u, a.v_[1] > b.v_[1] ? 0xFFFFFFFFu : 0u,
                         a.v_[2] > b.v_[2] ? 0xFFFFFFFFu : 0u, a.v_[3] > b.v_[3] ? 0xFFFFFFFFu : 0u}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{{a.v_[0] >= b.v_[0] ? 0xFFFFFFFFu : 0u, a.v_[1] >= b.v_[1] ? 0xFFFFFFFFu : 0u,
                         a.v_[2] >= b.v_[2] ? 0xFFFFFFFFu : 0u, a.v_[3] >= b.v_[3] ? 0xFFFFFFFFu : 0u}};
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAllTrue(SimdMask32x4 m) noexcept
{
    return (m.v_[0] & m.v_[1] & m.v_[2] & m.v_[3]) == 0xFFFFFFFFu;
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAnyTrue(SimdMask32x4 m) noexcept
{
    return (m.v_[0] | m.v_[1] | m.v_[2] | m.v_[3]) != 0u;
}

// ----- Logical / select ---------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAnd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    SimdF32x4 r;
    std::uint32_t ai[4], bi[4], ri[4];
    std::memcpy(ai, a.v_, 16);
    std::memcpy(bi, b.v_, 16);
    ri[0] = ai[0] & bi[0];
    ri[1] = ai[1] & bi[1];
    ri[2] = ai[2] & bi[2];
    ri[3] = ai[3] & bi[3];
    std::memcpy(r.v_, ri, 16);
    return r;
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdOr(SimdF32x4 a, SimdF32x4 b) noexcept
{
    SimdF32x4 r;
    std::uint32_t ai[4], bi[4], ri[4];
    std::memcpy(ai, a.v_, 16);
    std::memcpy(bi, b.v_, 16);
    ri[0] = ai[0] | bi[0];
    ri[1] = ai[1] | bi[1];
    ri[2] = ai[2] | bi[2];
    ri[3] = ai[3] | bi[3];
    std::memcpy(r.v_, ri, 16);
    return r;
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdXor(SimdF32x4 a, SimdF32x4 b) noexcept
{
    SimdF32x4 r;
    std::uint32_t ai[4], bi[4], ri[4];
    std::memcpy(ai, a.v_, 16);
    std::memcpy(bi, b.v_, 16);
    ri[0] = ai[0] ^ bi[0];
    ri[1] = ai[1] ^ bi[1];
    ri[2] = ai[2] ^ bi[2];
    ri[3] = ai[3] ^ bi[3];
    std::memcpy(r.v_, ri, 16);
    return r;
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAndNot(SimdF32x4 a, SimdF32x4 b) noexcept
{
    SimdF32x4 r;
    std::uint32_t ai[4], bi[4], ri[4];
    std::memcpy(ai, a.v_, 16);
    std::memcpy(bi, b.v_, 16);
    ri[0] = ~ai[0] & bi[0];
    ri[1] = ~ai[1] & bi[1];
    ri[2] = ~ai[2] & bi[2];
    ri[3] = ~ai[3] & bi[3];
    std::memcpy(r.v_, ri, 16);
    return r;
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSelect(SimdMask32x4 mask, SimdF32x4 ifTrue, SimdF32x4 ifFalse) noexcept
{
    SimdF32x4 r;
    std::uint32_t ti[4], fi[4], ri[4];
    std::memcpy(ti, ifTrue.v_, 16);
    std::memcpy(fi, ifFalse.v_, 16);
    ri[0] = (ti[0] & mask.v_[0]) | (fi[0] & ~mask.v_[0]);
    ri[1] = (ti[1] & mask.v_[1]) | (fi[1] & ~mask.v_[1]);
    ri[2] = (ti[2] & mask.v_[2]) | (fi[2] & ~mask.v_[2]);
    ri[3] = (ti[3] & mask.v_[3]) | (fi[3] & ~mask.v_[3]);
    std::memcpy(r.v_, ri, 16);
    return r;
}

// ----- Shuffle ------------------------------------------------------------------

template<int I0, int I1, int I2, int I3> [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle(SimdF32x4 v) noexcept
{
    static_assert(I0 >= 0 && I0 < 4 && I1 >= 0 && I1 < 4 && I2 >= 0 && I2 < 4 && I3 >= 0 && I3 < 4,
                  "SimdShuffle lane indices must be in [0, 4)");
    return SimdF32x4{{v.v_[I0], v.v_[I1], v.v_[I2], v.v_[I3]}};
}

template<int A0, int A1, int B0, int B1>
[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle2(SimdF32x4 a, SimdF32x4 b) noexcept
{
    static_assert(A0 >= 0 && A0 < 4 && A1 >= 0 && A1 < 4 && B0 >= 0 && B0 < 4 && B1 >= 0 && B1 < 4,
                  "SimdShuffle2 lane indices must be in [0, 4)");
    return SimdF32x4{{a.v_[A0], a.v_[A1], b.v_[B0], b.v_[B1]}};
}

// ----- Horizontal ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot4(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const float d = a.v_[0] * b.v_[0] + a.v_[1] * b.v_[1] + a.v_[2] * b.v_[2] + a.v_[3] * b.v_[3];
    return SimdF32x4{{d, d, d, d}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const float d = a.v_[0] * b.v_[0] + a.v_[1] * b.v_[1] + a.v_[2] * b.v_[2];
    return SimdF32x4{{d, d, d, d}};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdCross3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{{a.v_[1] * b.v_[2] - a.v_[2] * b.v_[1], a.v_[2] * b.v_[0] - a.v_[0] * b.v_[2],
                      a.v_[0] * b.v_[1] - a.v_[1] * b.v_[0], 0.0f}};
}

} // namespace Hylux::Math
