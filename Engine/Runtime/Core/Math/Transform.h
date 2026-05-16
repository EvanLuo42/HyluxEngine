/// @file
/// @brief Compact TRS (translation + rotation + scale) transform.

#pragma once

#include "Core/Math/Mat4.h"
#include "Core/Math/Quat.h"
#include "Core/Math/Vec3.h"

namespace Hylux::Math
{

/// @brief Affine transform stored as separate translation / rotation / scale,
///        composable on its own without materializing a Mat4.
class Transform
{
public:
    HYLUX_FORCEINLINE Transform() noexcept
        : translation_(Vec3::Zero()), rotation_(Quat::Identity()), scale_(Vec3::One())
    {}

    HYLUX_FORCEINLINE Transform(Vec3 translation, Quat rotation, Vec3 scale) noexcept
        : translation_(translation), rotation_(rotation), scale_(scale)
    {}

    [[nodiscard]] HYLUX_FORCEINLINE Vec3 Translation() const noexcept { return translation_; }
    [[nodiscard]] HYLUX_FORCEINLINE Quat Rotation() const noexcept { return rotation_; }
    [[nodiscard]] HYLUX_FORCEINLINE Vec3 Scale() const noexcept { return scale_; }

    HYLUX_FORCEINLINE void SetTranslation(Vec3 v) noexcept { translation_ = v; }
    HYLUX_FORCEINLINE void SetRotation(Quat q) noexcept { rotation_ = q; }
    HYLUX_FORCEINLINE void SetScale(Vec3 s) noexcept { scale_ = s; }

    /// @brief Materializes the transform as a 4x4 matrix: T * R * S.
    [[nodiscard]] HYLUX_FORCEINLINE Mat4 ToMat4() const noexcept
    {
        return Mat4::Translation(translation_) * rotation_.ToMat4() * Mat4::Scale(scale_);
    }

    /// @brief Applies the transform to a point (includes translation).
    [[nodiscard]] HYLUX_FORCEINLINE Vec3 TransformPoint(Vec3 p) const noexcept
    {
        return translation_ + rotation_.Rotate(scale_ * p);
    }

    /// @brief Applies the transform to a direction (no translation, no scale-shear correction).
    [[nodiscard]] HYLUX_FORCEINLINE Vec3 TransformDirection(Vec3 d) const noexcept { return rotation_.Rotate(d); }

private:
    Vec3 translation_;
    Quat rotation_;
    Vec3 scale_;
};

} // namespace Hylux::Math
