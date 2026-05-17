/// @file
/// @brief Tests for the RenderGraph public value-types: SSA handles, access descriptors,
///        and the reflected RGPass base class. The full Compile/Execute path requires a
///        live IRHIDevice and is covered by backend integration tests, not here.

#include "Core/Reflection/TypeInfo.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RGRasterPass.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <type_traits>

using namespace Hylux;
using namespace Hylux::RG;

// ---- RGTextureHandle --------------------------------------------------------

TEST_CASE("RGTextureHandle is default-invalid")
{
    constexpr RGTextureHandle handle;
    static_assert(!handle.IsValid());
    static_assert(handle.Index() == kInvalidRGIndex);
    static_assert(handle.Version() == 0u);
    CHECK_FALSE(handle.IsValid());
}

TEST_CASE("RGTextureHandle equality compares both index and version")
{
    constexpr RGTextureHandle a;
    constexpr RGTextureHandle b;
    static_assert(a == b);
    static_assert(!(a != b));
    CHECK(a == b);
}

// ---- RGBufferHandle ---------------------------------------------------------

TEST_CASE("RGBufferHandle is default-invalid")
{
    constexpr RGBufferHandle handle;
    static_assert(!handle.IsValid());
    static_assert(handle.Index() == kInvalidRGIndex);
    static_assert(handle.Version() == 0u);
    CHECK_FALSE(handle.IsValid());
}

TEST_CASE("RGBufferHandle equality compares both index and version")
{
    constexpr RGBufferHandle a;
    constexpr RGBufferHandle b;
    static_assert(a == b);
    CHECK(a == b);
}

// ---- Sentinel ---------------------------------------------------------------

TEST_CASE("kInvalidRGIndex is the all-ones sentinel")
{
    static_assert(kInvalidRGIndex == 0xFFFFFFFFu);
    CHECK(kInvalidRGIndex == 0xFFFFFFFFu);
}

// ---- RGTextureAccess defaults ----------------------------------------------

TEST_CASE("RGTextureAccess defaults model a pixel-shader sampled read")
{
    constexpr RGTextureAccess access;
    CHECK(access.layout == RHI::ImageLayout::ShaderReadOnly);
    CHECK(access.stages == RHI::PipelineStageMask::PixelShader);
    CHECK(access.access == RHI::AccessMask::ShaderResourceRead);
}

TEST_CASE("RGTextureAccess can be brace-initialized to override the layout")
{
    constexpr RGTextureAccess colorWrite{
        RHI::ImageLayout::ColorAttachment,
        RHI::PipelineStageMask::ColorAttachmentOutput,
        RHI::AccessMask::ColorAttachmentWrite,
    };
    CHECK(colorWrite.layout == RHI::ImageLayout::ColorAttachment);
    CHECK(colorWrite.access == RHI::AccessMask::ColorAttachmentWrite);
}

// ---- RGBufferAccess defaults ------------------------------------------------

TEST_CASE("RGBufferAccess defaults model a compute-shader structured read")
{
    constexpr RGBufferAccess access;
    CHECK(access.stages == RHI::PipelineStageMask::ComputeShader);
    CHECK(access.access == RHI::AccessMask::ShaderResourceRead);
}

// ---- RGPass reflection ------------------------------------------------------

TEST_CASE("RGPass is registered with the reflection system")
{
    const TypeInfo* info = RGPass::GetStaticType();
    REQUIRE(info != nullptr);
    CHECK(info->Id() == TypeIdOf<RGPass>());
    CHECK(info->Size() == sizeof(RGPass));
}

TEST_CASE("RGRasterPass is registered and reports RGPass as its reflected base")
{
    const TypeInfo* raster = RGRasterPass::GetStaticType();
    const TypeInfo* base   = RGPass::GetStaticType();
    REQUIRE(raster != nullptr);
    REQUIRE(base != nullptr);
    CHECK(raster->IsA(base));
    CHECK_FALSE(base->IsA(raster));
}

// ---- Compile-time guarantees ------------------------------------------------

static_assert(std::is_trivially_copyable_v<RGTextureHandle>,
              "RGTextureHandle is meant to be passed by value");
static_assert(std::is_trivially_copyable_v<RGBufferHandle>,
              "RGBufferHandle is meant to be passed by value");
