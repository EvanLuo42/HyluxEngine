/// @file
/// @brief SSE4.1 implementation of the L0 SIMD API.

#pragma once

#include "Core/Math/Detail/PlatformDefines.h"

#include <cstdint>
#include <smmintrin.h>

namespace Hylux::Math
{

struct HYLUX_ALIGN(16) SimdF32x4
{
    __m128 v_;
};

struct HYLUX_ALIGN(16) SimdI32x4
{
    __m128i v_;
};

struct HYLUX_ALIGN(16) SimdMask32x4
{
    __m128 v_;
};

// ----- Construction / conversion ------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadAligned(const float* src) noexcept
{
    return SimdF32x4{_mm_load_ps(src)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdLoadUnaligned(const float* src) noexcept
{
    return SimdF32x4{_mm_loadu_ps(src)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSet(float x, float y, float z, float w) noexcept
{
    return SimdF32x4{_mm_setr_ps(x, y, z, w)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSplat(float v) noexcept
{
    return SimdF32x4{_mm_set1_ps(v)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdZero() noexcept
{
    return SimdF32x4{_mm_setzero_ps()};
}

HYLUX_FORCEINLINE void SimdStoreAligned(float* dst, SimdF32x4 v) noexcept
{
    _mm_store_ps(dst, v.v_);
}

HYLUX_FORCEINLINE void SimdStoreUnaligned(float* dst, SimdF32x4 v) noexcept
{
    _mm_storeu_ps(dst, v.v_);
}

[[nodiscard]] HYLUX_FORCEINLINE float SimdGetX(SimdF32x4 v) noexcept
{
    return _mm_cvtss_f32(v.v_);
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetY(SimdF32x4 v) noexcept
{
    return _mm_cvtss_f32(_mm_shuffle_ps(v.v_, v.v_, _MM_SHUFFLE(1, 1, 1, 1)));
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetZ(SimdF32x4 v) noexcept
{
    return _mm_cvtss_f32(_mm_shuffle_ps(v.v_, v.v_, _MM_SHUFFLE(2, 2, 2, 2)));
}
[[nodiscard]] HYLUX_FORCEINLINE float SimdGetW(SimdF32x4 v) noexcept
{
    return _mm_cvtss_f32(_mm_shuffle_ps(v.v_, v.v_, _MM_SHUFFLE(3, 3, 3, 3)));
}

// ----- Arithmetic ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAdd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_add_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSub(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_sub_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMul(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_mul_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDiv(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_div_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdNeg(SimdF32x4 a) noexcept
{
    return SimdF32x4{_mm_sub_ps(_mm_setzero_ps(), a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMin(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_min_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMax(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_max_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdMulAdd(SimdF32x4 a, SimdF32x4 b, SimdF32x4 c) noexcept
{
#if defined(__FMA__)
    return SimdF32x4{_mm_fmadd_ps(a.v_, b.v_, c.v_)};
#else
    return SimdF32x4{_mm_add_ps(_mm_mul_ps(a.v_, b.v_), c.v_)};
#endif
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRcp(SimdF32x4 a) noexcept
{
    return SimdF32x4{_mm_div_ps(_mm_set1_ps(1.0f), a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{_mm_sqrt_ps(a.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdRsqrt(SimdF32x4 a) noexcept
{
    return SimdF32x4{_mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(a.v_))};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAbs(SimdF32x4 a) noexcept
{
    const __m128 signMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    return SimdF32x4{_mm_and_ps(a.v_, signMask)};
}

// ----- Compare ------------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpEq(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{_mm_cmpeq_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{_mm_cmplt_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpLe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{_mm_cmple_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGt(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{_mm_cmpgt_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdMask32x4 SimdCmpGe(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdMask32x4{_mm_cmpge_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAllTrue(SimdMask32x4 m) noexcept
{
    return _mm_movemask_ps(m.v_) == 0xF;
}

[[nodiscard]] HYLUX_FORCEINLINE bool SimdAnyTrue(SimdMask32x4 m) noexcept
{
    return _mm_movemask_ps(m.v_) != 0;
}

// ----- Logical / select ---------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAnd(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_and_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdOr(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_or_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdXor(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_xor_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdAndNot(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_andnot_ps(a.v_, b.v_)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdSelect(SimdMask32x4 mask, SimdF32x4 ifTrue, SimdF32x4 ifFalse) noexcept
{
    return SimdF32x4{_mm_blendv_ps(ifFalse.v_, ifTrue.v_, mask.v_)};
}

// ----- Shuffle ------------------------------------------------------------------

template<int I0, int I1, int I2, int I3> [[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle(SimdF32x4 v) noexcept
{
    static_assert(I0 >= 0 && I0 < 4 && I1 >= 0 && I1 < 4 && I2 >= 0 && I2 < 4 && I3 >= 0 && I3 < 4,
                  "SimdShuffle lane indices must be in [0, 4)");
    return SimdF32x4{_mm_shuffle_ps(v.v_, v.v_, _MM_SHUFFLE(I3, I2, I1, I0))};
}

template<int A0, int A1, int B0, int B1>
[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdShuffle2(SimdF32x4 a, SimdF32x4 b) noexcept
{
    static_assert(A0 >= 0 && A0 < 4 && A1 >= 0 && A1 < 4 && B0 >= 0 && B0 < 4 && B1 >= 0 && B1 < 4,
                  "SimdShuffle2 lane indices must be in [0, 4)");
    return SimdF32x4{_mm_shuffle_ps(a.v_, b.v_, _MM_SHUFFLE(B1, B0, A1, A0))};
}

// ----- Horizontal ---------------------------------------------------------------

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot4(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_dp_ps(a.v_, b.v_, 0xFF)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdDot3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    return SimdF32x4{_mm_dp_ps(a.v_, b.v_, 0x7F)};
}

[[nodiscard]] HYLUX_FORCEINLINE SimdF32x4 SimdCross3(SimdF32x4 a, SimdF32x4 b) noexcept
{
    const __m128 aYzx = _mm_shuffle_ps(a.v_, a.v_, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 bYzx = _mm_shuffle_ps(b.v_, b.v_, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 c = _mm_sub_ps(_mm_mul_ps(a.v_, bYzx), _mm_mul_ps(aYzx, b.v_));
    return SimdF32x4{_mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1))};
}

} // namespace Hylux::Math
