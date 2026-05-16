/// @file
/// @brief Out-of-line implementation for non-trivial Mat3 operations.

#include "Core/Math/Mat3.h"

namespace Hylux::Math
{

Mat3 operator*(const Mat3& a, const Mat3& b) noexcept
{
    auto composeColumn = [&](SimdF32x4 bj) {
        return SimdMulAdd(
            a.cols_[0], SimdSplat(SimdGetX(bj)),
            SimdMulAdd(a.cols_[1], SimdSplat(SimdGetY(bj)), SimdMul(a.cols_[2], SimdSplat(SimdGetZ(bj)))));
    };

    Mat3 r;
    r.cols_[0] = composeColumn(b.cols_[0]);
    r.cols_[1] = composeColumn(b.cols_[1]);
    r.cols_[2] = composeColumn(b.cols_[2]);
    return r;
}

Mat3 Mat3::Transposed() const noexcept
{
    alignas(16) float buf[12];
    SimdStoreAligned(buf + 0, cols_[0]);
    SimdStoreAligned(buf + 4, cols_[1]);
    SimdStoreAligned(buf + 8, cols_[2]);

    Mat3 r;
    r.cols_[0] = SimdSet(buf[0], buf[4], buf[8], 0.0f);
    r.cols_[1] = SimdSet(buf[1], buf[5], buf[9], 0.0f);
    r.cols_[2] = SimdSet(buf[2], buf[6], buf[10], 0.0f);
    return r;
}

} // namespace Hylux::Math
