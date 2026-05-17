/// @file
/// @brief Tests for RGTextureHandle / RGBufferHandle value semantics.

#include "RenderGraph/RGHandle.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RenderGraph");

using namespace Hylux::RG;

TEST_CASE("RGTextureHandle: default is invalid; equality covers index + version")
{
    constexpr RGTextureHandle a;
    static_assert(!a.IsValid());
    CHECK(a.Index() == kInvalidRGIndex);
    CHECK(a.Version() == 0u);
    CHECK(a == RGTextureHandle{});
    CHECK_FALSE(a != RGTextureHandle{});
}

TEST_CASE("RGBufferHandle: default is invalid; equality and inequality respect both fields")
{
    constexpr RGBufferHandle a;
    static_assert(!a.IsValid());
    CHECK(a == RGBufferHandle{});
}

TEST_CASE("RGHandle: kInvalidRGIndex sentinel")
{
    static_assert(kInvalidRGIndex == 0xFFFFFFFFu);
}

TEST_SUITE_END();
