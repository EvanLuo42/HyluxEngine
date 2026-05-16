/// @file
/// @brief 2-component float vector. Scalar storage (8 bytes, no SIMD).

#pragma once

#include "Core/Math/Common.h"

namespace Hylux::Math
{

/// @brief 2-component float vector. Plain scalar storage (no SIMD benefit at 8 bytes).
class Vec2
{
public:
    constexpr Vec2() noexcept : x_(0.0f), y_(0.0f) {}
    constexpr Vec2(float x, float y) noexcept : x_(x), y_(y) {}
    constexpr explicit Vec2(float splat) noexcept : x_(splat), y_(splat) {}

    [[nodiscard]] constexpr float X() const noexcept { return x_; }
    [[nodiscard]] constexpr float Y() const noexcept { return y_; }

    [[nodiscard]] static constexpr float Dot(Vec2 a, Vec2 b) noexcept { return a.x_ * b.x_ + a.y_ * b.y_; }

    [[nodiscard]] constexpr float LengthSquared() const noexcept { return x_ * x_ + y_ * y_; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] Vec2 Normalized() const noexcept
    {
        const float len = Length();
        if (len <= kEpsilon)
        {
            return Vec2{0.0f, 0.0f};
        }
        const float inv = 1.0f / len;
        return Vec2{x_ * inv, y_ * inv};
    }

    [[nodiscard]] static constexpr Vec2 Min(Vec2 a, Vec2 b) noexcept
    {
        return Vec2{a.x_ < b.x_ ? a.x_ : b.x_, a.y_ < b.y_ ? a.y_ : b.y_};
    }
    [[nodiscard]] static constexpr Vec2 Max(Vec2 a, Vec2 b) noexcept
    {
        return Vec2{a.x_ > b.x_ ? a.x_ : b.x_, a.y_ > b.y_ ? a.y_ : b.y_};
    }
    [[nodiscard]] static constexpr Vec2 Lerp(Vec2 a, Vec2 b, float t) noexcept
    {
        return Vec2{a.x_ + (b.x_ - a.x_) * t, a.y_ + (b.y_ - a.y_) * t};
    }

private:
    float x_;
    float y_;
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept
{
    return Vec2{a.X() + b.X(), a.Y() + b.Y()};
}
[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept
{
    return Vec2{a.X() - b.X(), a.Y() - b.Y()};
}
[[nodiscard]] constexpr Vec2 operator*(Vec2 a, Vec2 b) noexcept
{
    return Vec2{a.X() * b.X(), a.Y() * b.Y()};
}
[[nodiscard]] constexpr Vec2 operator*(Vec2 a, float s) noexcept
{
    return Vec2{a.X() * s, a.Y() * s};
}
[[nodiscard]] constexpr Vec2 operator*(float s, Vec2 a) noexcept
{
    return Vec2{a.X() * s, a.Y() * s};
}
[[nodiscard]] constexpr Vec2 operator/(Vec2 a, float s) noexcept
{
    return Vec2{a.X() / s, a.Y() / s};
}
[[nodiscard]] constexpr Vec2 operator-(Vec2 a) noexcept
{
    return Vec2{-a.X(), -a.Y()};
}
[[nodiscard]] constexpr bool operator==(Vec2 a, Vec2 b) noexcept
{
    return a.X() == b.X() && a.Y() == b.Y();
}
[[nodiscard]] constexpr bool operator!=(Vec2 a, Vec2 b) noexcept
{
    return !(a == b);
}

static_assert(sizeof(Vec2) == 8, "Vec2 must be 8 bytes");

} // namespace Hylux::Math
