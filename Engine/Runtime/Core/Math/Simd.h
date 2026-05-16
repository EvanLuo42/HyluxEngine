/// @file
/// @brief L0 SIMD facade. Selects a backend implementation and exposes the
///        opaque vector types and free-function intrinsics used by all higher
///        math types. Console platforms override by defining
///        HYLUX_MATH_SIMD_BACKEND_EXT and HYLUX_MATH_SIMD_EXTENSION_HEADER.

#pragma once

#include "Core/Math/Detail/SimdSelect.h"

#if defined(HYLUX_MATH_SIMD_BACKEND_EXT)
#include HYLUX_MATH_SIMD_EXTENSION_HEADER
#elif defined(HYLUX_MATH_SIMD_BACKEND_SSE)
#include "Core/Math/Detail/SimdSse.inl"
#elif defined(HYLUX_MATH_SIMD_BACKEND_NEON)
#include "Core/Math/Detail/SimdNeon.inl"
#elif defined(HYLUX_MATH_SIMD_BACKEND_SCALAR)
#include "Core/Math/Detail/SimdScalar.inl"
#else
#error "No Hylux math SIMD backend was selected (this should not happen if SimdSelect.h ran)."
#endif

namespace Hylux::Math
{

static_assert(sizeof(SimdF32x4) == 16, "SimdF32x4 must be 16 bytes");
static_assert(alignof(SimdF32x4) == 16, "SimdF32x4 must be 16-byte aligned");
static_assert(sizeof(SimdI32x4) == 16, "SimdI32x4 must be 16 bytes");
static_assert(alignof(SimdI32x4) == 16, "SimdI32x4 must be 16-byte aligned");
static_assert(sizeof(SimdMask32x4) == 16, "SimdMask32x4 must be 16 bytes");
static_assert(alignof(SimdMask32x4) == 16, "SimdMask32x4 must be 16-byte aligned");

} // namespace Hylux::Math
