/// @file
/// @brief Out-of-line Mat3x4 methods.

#include "Core/Math/Mat3x4.h"

namespace Hylux::Math
{

Mat3x4 Mat3x4::FromMat4(const Mat4& m4) noexcept
{
    // m4 is column-major: cols[j].i = M[i][j]. To extract row i we gather component i
    // from every column. The implicit bottom row of a Mat3x4 is (0, 0, 0, 1); we drop
    // any contents M[3][*] the source may carry.
    const Vec4 c0 = m4.Column(0);
    const Vec4 c1 = m4.Column(1);
    const Vec4 c2 = m4.Column(2);
    const Vec4 c3 = m4.Column(3);

    Mat3x4 result;
    result.rows_[0] = SimdSet(c0.X(), c1.X(), c2.X(), c3.X());
    result.rows_[1] = SimdSet(c0.Y(), c1.Y(), c2.Y(), c3.Y());
    result.rows_[2] = SimdSet(c0.Z(), c1.Z(), c2.Z(), c3.Z());
    return result;
}

} // namespace Hylux::Math
