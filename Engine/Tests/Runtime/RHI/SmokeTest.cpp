/// @file
/// @brief Compile and runtime sanity checks for the RHI public API surface. The smoke test
///        constructs descriptors, exercises the FeatureSet bitmap, and asserts the layout
///        of small POD types. No backend is invoked.

#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIInstance.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHIInstanceDesc.h"
#include "RHI/RHINativeHandle.h"
#include "RHI/RHIPipelineDesc.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIValidation.h"

#include <doctest/doctest.h>

#include <cstdint>

using namespace Hylux::RHI;

static_assert(sizeof(RHINativeHandle) <= 16, "RHINativeHandle must remain a small POD");
static_assert(kMaxPushConstantSize == 128, "kMaxPushConstantSize must be 128 bytes");
static_assert(static_cast<std::uint32_t>(BindlessIndex::Invalid) == 0xFFFFFFFFu,
              "BindlessIndex::Invalid sentinel must be 0xFFFFFFFFu");

TEST_CASE("RHINativeHandle default state is None / 0")
{
    RHINativeHandle handle{};
    CHECK(handle.kind == RHINativeHandleKind::None);
    CHECK(handle.value == 0u);
}

TEST_CASE("RHINativeHandle can carry a tagged backend pointer")
{
    RHINativeHandle handle{RHINativeHandleKind::VkBuffer, 0xDEADBEEF12345678ull};
    CHECK(handle.kind == RHINativeHandleKind::VkBuffer);
    CHECK(handle.value == 0xDEADBEEF12345678ull);
}

TEST_CASE("FeatureSet set / clear / has round trip")
{
    FeatureSet features;
    CHECK(features.Empty());

    features.Set(Feature::Bindless);
    features.Set(Feature::DynamicRendering);

    CHECK(features.Has(Feature::Bindless));
    CHECK(features.Has(Feature::DynamicRendering));
    CHECK_FALSE(features.Has(Feature::RayTracing));
    CHECK(features.Count() == 2u);

    features.Clear(Feature::Bindless);
    CHECK_FALSE(features.Has(Feature::Bindless));
    CHECK(features.Count() == 1u);
}

TEST_CASE("FeatureSet initializer-list constructor populates flags")
{
    FeatureSet features{Feature::MeshShader, Feature::RayQuery, Feature::CaptureNsight};
    CHECK(features.Has(Feature::MeshShader));
    CHECK(features.Has(Feature::RayQuery));
    CHECK(features.Has(Feature::CaptureNsight));
    CHECK(features.Count() == 3u);
}

TEST_CASE("FeatureSet supports union / intersection")
{
    const FeatureSet a{Feature::Bindless, Feature::RayTracing};
    const FeatureSet b{Feature::Bindless, Feature::MeshShader};

    const FeatureSet unionSet = a | b;
    CHECK(unionSet.Has(Feature::Bindless));
    CHECK(unionSet.Has(Feature::RayTracing));
    CHECK(unionSet.Has(Feature::MeshShader));
    CHECK(unionSet.Count() == 3u);

    const FeatureSet intersection = a & b;
    CHECK(intersection.Has(Feature::Bindless));
    CHECK_FALSE(intersection.Has(Feature::RayTracing));
    CHECK_FALSE(intersection.Has(Feature::MeshShader));
    CHECK(intersection.Count() == 1u);
}

TEST_CASE("Validation levels have monotonic ordering")
{
    CHECK(RhiValidationLevel::Off      < RhiValidationLevel::Basic);
    CHECK(RhiValidationLevel::Basic    < RhiValidationLevel::Standard);
    CHECK(RhiValidationLevel::Standard < RhiValidationLevel::Strict);

    CHECK(GapiValidationLevel::Off      < GapiValidationLevel::Basic);
    CHECK(GapiValidationLevel::Basic    < GapiValidationLevel::Standard);
    CHECK(GapiValidationLevel::Standard < GapiValidationLevel::Gpu);
    CHECK(GapiValidationLevel::Gpu      < GapiValidationLevel::GpuAggressive);
}

TEST_CASE("BufferDesc and TextureDesc default values")
{
    BufferDesc buffer;
    CHECK(buffer.size == 0u);
    CHECK(buffer.usage == BufferUsage::None);
    CHECK(buffer.memory == MemoryUsage::GpuOnly);
    CHECK_FALSE(buffer.bindlessSrv);
    CHECK_FALSE(buffer.bindlessUav);
    CHECK_FALSE(buffer.bindlessCbv);

    TextureDesc texture;
    CHECK(texture.dimension == TextureDimension::Tex2D);
    CHECK(texture.format == Format::Unknown);
    CHECK(texture.mipLevels == 1u);
    CHECK(texture.arrayLayers == 1u);
    CHECK(texture.sampleCount == 1u);
    CHECK(texture.memory == MemoryUsage::GpuOnly);
}

TEST_CASE("SamplerDesc default state")
{
    SamplerDesc sampler;
    CHECK(sampler.minFilter == FilterMode::Linear);
    CHECK(sampler.magFilter == FilterMode::Linear);
    CHECK(sampler.mipFilter == MipFilterMode::Linear);
    CHECK(sampler.addressU == AddressMode::Repeat);
    CHECK(sampler.addressV == AddressMode::Repeat);
    CHECK(sampler.addressW == AddressMode::Repeat);
    CHECK_FALSE(sampler.compareEnable);
}

TEST_CASE("InstanceDesc defaults to safe values")
{
    InstanceDesc desc;
    CHECK(desc.preferredDevice == DeviceType::Vulkan);
    CHECK(desc.rhiValidation == RhiValidationLevel::Off);
    CHECK(desc.gapiValidation == GapiValidationLevel::Off);
    CHECK(desc.captureTool == CaptureToolKind::Auto);
}

TEST_CASE("DeviceDesc has sane bindless heap defaults")
{
    DeviceDesc desc;
    CHECK(desc.graphicsQueueCount == 1u);
    CHECK(desc.bindlessSizes.srvCbvUavCapacity > 0u);
    CHECK(desc.bindlessSizes.samplerCapacity > 0u);
}

TEST_CASE("BufferUsage and ShaderStage support bitwise composition")
{
    const BufferUsage combined = BufferUsage::VertexBuffer | BufferUsage::IndexBuffer;
    CHECK((combined & BufferUsage::VertexBuffer) == BufferUsage::VertexBuffer);
    CHECK((combined & BufferUsage::IndexBuffer) == BufferUsage::IndexBuffer);
    CHECK((combined & BufferUsage::UniformBuffer) == BufferUsage::None);

    const ShaderStage gfx = ShaderStage::Vertex | ShaderStage::Pixel;
    CHECK((gfx & ShaderStage::Vertex) == ShaderStage::Vertex);
    CHECK((gfx & ShaderStage::Compute) == ShaderStage::None);
}

TEST_CASE("Barrier masks compose with bitwise operators")
{
    const AccessMask access = AccessMask::ShaderResourceRead | AccessMask::ShaderResourceWrite;
    CHECK((access & AccessMask::ShaderResourceRead) == AccessMask::ShaderResourceRead);
    CHECK((access & AccessMask::TransferRead) == AccessMask::None);

    const PipelineStageMask stages = PipelineStageMask::ComputeShader | PipelineStageMask::Transfer;
    CHECK((stages & PipelineStageMask::ComputeShader) == PipelineStageMask::ComputeShader);
    CHECK((stages & PipelineStageMask::PixelShader) == PipelineStageMask::None);
}

TEST_CASE("Format helpers classify depth and sRGB variants")
{
    CHECK(FormatHasDepth(Format::D24UnormS8Uint));
    CHECK(FormatHasStencil(Format::D24UnormS8Uint));
    CHECK(FormatHasDepth(Format::D32Float));
    CHECK_FALSE(FormatHasStencil(Format::D32Float));

    CHECK(FormatIsSrgb(Format::RGBA8UnormSrgb));
    CHECK(FormatIsSrgb(Format::BC7UnormSrgb));
    CHECK_FALSE(FormatIsSrgb(Format::RGBA8Unorm));

    CHECK(FormatIsCompressed(Format::BC7UnormSrgb));
    CHECK(FormatIsCompressed(Format::Astc4x4Unorm));
    CHECK_FALSE(FormatIsCompressed(Format::RGBA8Unorm));
}
