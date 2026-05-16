/// @file
/// @brief Scalar constants and utility functions shared across the Hylux math
///        library. Header-only, constexpr where the standard library permits.

#pragma once

#include "Core/Math/Detail/PlatformDefines.h"

#include <cmath>
#include <limits>

namespace Hylux::Math
{

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;
inline constexpr float kHalfPi = 1.57079632679489661923f;
inline constexpr float kInvPi = 0.31830988618379067154f;
inline constexpr float kDegToRad = kPi / 180.0f;
inline constexpr float kRadToDeg = 180.0f / kPi;
inline constexpr float kEpsilon = 1.0e-6f;
inline constexpr float kLargeEpsilon = 1.0e-4f;

/// @brief Linearly interpolates from a to b by t (t is not clamped).
template<typename T> [[nodiscard]] HYLUX_FORCEINLINE constexpr T Lerp(T a, T b, T t) noexcept
{
    return a + (b - a) * t;
}

/// @brief Clamps v into the inclusive range [lo, hi].
template<typename T> [[nodiscard]] HYLUX_FORCEINLINE constexpr T Clamp(T v, T lo, T hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/// @brief Clamps v into [0, 1].
[[nodiscard]] HYLUX_FORCEINLINE constexpr float Saturate(float v) noexcept
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/// @brief Degrees to radians.
[[nodiscard]] HYLUX_FORCEINLINE constexpr float DegToRad(float degrees) noexcept
{
    return degrees * kDegToRad;
}

/// @brief Radians to degrees.
[[nodiscard]] HYLUX_FORCEINLINE constexpr float RadToDeg(float radians) noexcept
{
    return radians * kRadToDeg;
}

/// @brief Returns true when |a - b| <= tolerance.
[[nodiscard]] HYLUX_FORCEINLINE bool IsNearlyEqual(float a, float b, float tolerance = kEpsilon) noexcept
{
    return std::fabs(a - b) <= tolerance;
}

/// @brief Returns true when |v| <= tolerance.
[[nodiscard]] HYLUX_FORCEINLINE bool IsNearlyZero(float v, float tolerance = kEpsilon) noexcept
{
    return std::fabs(v) <= tolerance;
}

/// @brief Square (v * v). Useful in distance / energy expressions.
template<typename T> [[nodiscard]] HYLUX_FORCEINLINE constexpr T Square(T v) noexcept
{
    return v * v;
}

/// @brief Smoothstep (Hermite) interpolation between edges.
[[nodiscard]] HYLUX_FORCEINLINE constexpr float SmoothStep(float edge0, float edge1, float x) noexcept
{
    const float t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

} // namespace Hylux::Math
