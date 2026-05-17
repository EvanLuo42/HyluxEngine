/// @file
/// @brief Tests for RHI format helpers (FormatHasDepth/Stencil/IsCompressed/IsSrgb).

#include "RHI/RHIFormat.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RHI");

using namespace Hylux::RHI;

TEST_CASE("FormatHasDepth: matches every depth-bearing format")
{
    CHECK(FormatHasDepth(Format::D16Unorm));
    CHECK(FormatHasDepth(Format::D24UnormS8Uint));
    CHECK(FormatHasDepth(Format::D32Float));
    CHECK(FormatHasDepth(Format::D32FloatS8Uint));
    CHECK_FALSE(FormatHasDepth(Format::RGBA8Unorm));
    CHECK_FALSE(FormatHasDepth(Format::Unknown));
}

TEST_CASE("FormatHasStencil: only the depth+stencil variants")
{
    CHECK(FormatHasStencil(Format::D24UnormS8Uint));
    CHECK(FormatHasStencil(Format::D32FloatS8Uint));
    CHECK_FALSE(FormatHasStencil(Format::D32Float));
    CHECK_FALSE(FormatHasStencil(Format::D16Unorm));
    CHECK_FALSE(FormatHasStencil(Format::RGBA8Unorm));
}

TEST_CASE("FormatIsCompressed: BC / ETC2 / EAC / ASTC range")
{
    CHECK(FormatIsCompressed(Format::BC1RgbaUnorm));
    CHECK(FormatIsCompressed(Format::BC7UnormSrgb));
    CHECK(FormatIsCompressed(Format::Etc2Rgb8Unorm));
    CHECK(FormatIsCompressed(Format::Eac11Snorm));
    CHECK(FormatIsCompressed(Format::Astc4x4Unorm));
    CHECK(FormatIsCompressed(Format::Astc12x12UnormSrgb));
    CHECK_FALSE(FormatIsCompressed(Format::Unknown));
    CHECK_FALSE(FormatIsCompressed(Format::RGBA8Unorm));
    CHECK_FALSE(FormatIsCompressed(Format::D32Float));
}

TEST_CASE("FormatIsSrgb: positive + negative samples across families")
{
    CHECK(FormatIsSrgb(Format::RGBA8UnormSrgb));
    CHECK(FormatIsSrgb(Format::BGRA8UnormSrgb));
    CHECK(FormatIsSrgb(Format::BC1RgbaUnormSrgb));
    CHECK(FormatIsSrgb(Format::BC2UnormSrgb));
    CHECK(FormatIsSrgb(Format::BC3UnormSrgb));
    CHECK(FormatIsSrgb(Format::BC7UnormSrgb));
    CHECK(FormatIsSrgb(Format::Etc2Rgb8UnormSrgb));
    CHECK(FormatIsSrgb(Format::Astc4x4UnormSrgb));
    CHECK(FormatIsSrgb(Format::Astc12x12UnormSrgb));

    CHECK_FALSE(FormatIsSrgb(Format::RGBA8Unorm));
    CHECK_FALSE(FormatIsSrgb(Format::BC1RgbaUnorm));
    CHECK_FALSE(FormatIsSrgb(Format::Astc4x4Unorm));
    CHECK_FALSE(FormatIsSrgb(Format::Unknown));
    CHECK_FALSE(FormatIsSrgb(Format::D32Float));
}

TEST_CASE("FormatSupport: bit constants compose")
{
    FormatSupport sup{};
    sup.bits = FormatSupport::kSampledImage | FormatSupport::kColorAttachment;
    CHECK((sup.bits & FormatSupport::kSampledImage) != 0u);
    CHECK((sup.bits & FormatSupport::kStorageImage) == 0u);
}

TEST_SUITE_END();
