/// @file
/// @brief Tests for descriptor-struct defaults across the RHI public surface (Instance,
///        Device, Buffer, Texture, Sampler, Native handle).

#include "RHI/RHIBarriers.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIInstanceDesc.h"
#include "RHI/RHINativeHandle.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIValidation.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RHI");

#include <cstdint>

using namespace Hylux::RHI;

TEST_CASE("RHINativeHandle: default-constructed is { None, 0 }")
{
    RHINativeHandle h{};
    CHECK(h.kind == RHINativeHandleKind::None);
    CHECK(h.value == 0u);
}

TEST_CASE("RHINativeHandle: layout fits in 16 bytes")
{
    static_assert(sizeof(RHINativeHandle) <= 16);
}

TEST_CASE("BindlessIndex::Invalid sentinel is 0xFFFFFFFF")
{
    static_assert(static_cast<std::uint32_t>(BindlessIndex::Invalid) == 0xFFFFFFFFu);
}

TEST_CASE("kMaxPushConstantSize == 128; kRemainingLevels / kWholeSize sentinels")
{
    static_assert(kMaxPushConstantSize == 128u);
    static_assert(kRemainingLevels == 0xFFFFFFFFu);
    static_assert(kWholeSize == 0xFFFFFFFFFFFFFFFFull);
    static_assert(kMaxColorAttachments == 8u);
    static_assert(kQueueFamilyIgnored == 0xFFFFFFFFu);
}

TEST_CASE("InstanceDesc defaults: Vulkan, validation off, auto capture tool")
{
    InstanceDesc d;
    CHECK(d.preferredDevice == DeviceType::Vulkan);
    CHECK(d.rhiValidation == RhiValidationLevel::Off);
    CHECK(d.gapiValidation == GapiValidationLevel::Off);
    CHECK(d.captureTool == CaptureToolKind::Auto);
    CHECK(d.engineName == "HyluxEngine");
    CHECK(d.applicationVersion == 0u);
    CHECK(d.requiredFeatures.Empty());
    CHECK(d.desiredFeatures.Empty());
}

TEST_CASE("AdapterDesc default: Null / no flags")
{
    AdapterDesc d;
    CHECK(d.type == DeviceType::Null);
    CHECK(d.flags == AdapterFlags::None);
    CHECK(d.vendorId == 0u);
}

TEST_CASE("DeviceDesc defaults: queues + bindless heap sizes")
{
    DeviceDesc d;
    CHECK(d.graphicsQueueCount == 1u);
    CHECK(d.computeQueueCount == 1u);
    CHECK(d.copyQueueCount == 1u);
    CHECK(d.bindlessSizes.srvCbvUavCapacity > 0u);
    CHECK(d.bindlessSizes.samplerCapacity > 0u);
    CHECK(d.crashReporter == GpuCrashReporterKind::Auto);
}

TEST_CASE("BufferDesc default: zero size, GpuOnly memory, no bindless flags")
{
    BufferDesc b;
    CHECK(b.size == 0u);
    CHECK(b.usage == BufferUsage::None);
    CHECK(b.memory == MemoryUsage::GpuOnly);
    CHECK(b.structureStride == 0u);
    CHECK_FALSE(b.bindlessSrv);
    CHECK_FALSE(b.bindlessUav);
    CHECK_FALSE(b.bindlessCbv);
    CHECK_FALSE(b.aliasable);
}

TEST_CASE("TextureDesc default: 2D / Unknown / 1 mip / 1 array layer")
{
    TextureDesc t;
    CHECK(t.dimension == TextureDimension::Tex2D);
    CHECK(t.format == Format::Unknown);
    CHECK(t.mipLevels == 1u);
    CHECK(t.arrayLayers == 1u);
    CHECK(t.sampleCount == 1u);
    CHECK(t.memory == MemoryUsage::GpuOnly);
    CHECK_FALSE(t.aliasable);
}

TEST_CASE("TextureViewDesc default: 2D SRV; not RTV/UAV/DSV")
{
    TextureViewDesc v;
    CHECK(v.dimension == TextureDimension::Tex2D);
    CHECK(v.format == Format::Unknown);
    CHECK(v.isShaderResourceView);
    CHECK_FALSE(v.isUnorderedAccessView);
    CHECK_FALSE(v.isRenderTargetView);
    CHECK_FALSE(v.isDepthStencilView);
}

TEST_CASE("SamplerDesc default: trilinear, repeat, no compare")
{
    SamplerDesc s;
    CHECK(s.minFilter == FilterMode::Linear);
    CHECK(s.magFilter == FilterMode::Linear);
    CHECK(s.mipFilter == MipFilterMode::Linear);
    CHECK(s.addressU == AddressMode::Repeat);
    CHECK(s.addressV == AddressMode::Repeat);
    CHECK(s.addressW == AddressMode::Repeat);
    CHECK_FALSE(s.compareEnable);
    CHECK(s.compareOp == CompareOp::Never);
    CHECK(s.minLod == 0.0f);
    CHECK(s.maxLod > 0.0f);
    CHECK(s.maxAnisotropy == 1.0f);
}

TEST_CASE("SubresourceRange / ClearValue / Viewport defaults")
{
    SubresourceRange r;
    CHECK(r.baseMipLevel == 0u);
    CHECK(r.mipLevelCount == 1u);
    CHECK(r.baseArrayLayer == 0u);
    CHECK(r.arrayLayerCount == 1u);

    ClearValue cv;
    CHECK_FALSE(cv.isDepthStencil);
    CHECK(cv.color.r == 0.0f);
    CHECK(cv.depthStencil.depth == 1.0f);
    CHECK(cv.depthStencil.stencil == 0u);

    Viewport vp;
    CHECK(vp.minDepth == 0.0f);
    CHECK(vp.maxDepth == 1.0f);
}

TEST_CASE("BufferBarrier / TextureBarrier / MemoryBarrier defaults are no-op shaped")
{
    MemoryBarrier mb;
    CHECK(mb.srcStages == PipelineStageMask::None);
    CHECK(mb.dstAccess == AccessMask::None);

    BufferBarrier bb;
    CHECK(bb.buffer == nullptr);
    CHECK(bb.size == kWholeSize);
    CHECK(bb.srcQueueFamilyIndex == kQueueFamilyIgnored);
    CHECK(bb.dstQueueFamilyIndex == kQueueFamilyIgnored);

    TextureBarrier tb;
    CHECK(tb.texture == nullptr);
    CHECK(tb.oldLayout == ImageLayout::Undefined);
    CHECK(tb.newLayout == ImageLayout::Undefined);
}

TEST_SUITE_END();
