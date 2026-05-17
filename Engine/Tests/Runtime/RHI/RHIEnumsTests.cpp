/// @file
/// @brief Tests for RHI enum bitwise operators (ShaderStage, BufferUsage, TextureUsage,
///        SwapchainUsage, PipelineStageMask, AccessMask, CommandPoolFlags) and the
///        validation level orderings.

#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIValidation.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RHI");

using namespace Hylux::RHI;

TEST_CASE("ShaderStage: bitwise | and & compose flags")
{
    constexpr ShaderStage combined = ShaderStage::Vertex | ShaderStage::Pixel;
    static_assert((combined & ShaderStage::Vertex) == ShaderStage::Vertex);
    static_assert((combined & ShaderStage::Pixel) == ShaderStage::Pixel);
    static_assert((combined & ShaderStage::Compute) == ShaderStage::None);
    static_assert((ShaderStage::AllGraphics & ShaderStage::Vertex) == ShaderStage::Vertex);
    static_assert((ShaderStage::AllRayTracing & ShaderStage::Vertex) == ShaderStage::None);

    ShaderStage acc = ShaderStage::None;
    acc |= ShaderStage::Mesh;
    CHECK((acc & ShaderStage::Mesh) == ShaderStage::Mesh);
}

TEST_CASE("BufferUsage: bitwise composition matches mask")
{
    constexpr BufferUsage combined = BufferUsage::VertexBuffer | BufferUsage::IndexBuffer;
    static_assert((combined & BufferUsage::VertexBuffer) == BufferUsage::VertexBuffer);
    static_assert((combined & BufferUsage::UniformBuffer) == BufferUsage::None);
    BufferUsage acc = BufferUsage::None;
    acc |= BufferUsage::IndirectBuffer;
    CHECK((acc & BufferUsage::IndirectBuffer) == BufferUsage::IndirectBuffer);
}

TEST_CASE("TextureUsage / SwapchainUsage: bitwise composition")
{
    constexpr auto t = TextureUsage::SampledImage | TextureUsage::TransferDst;
    static_assert((t & TextureUsage::SampledImage) == TextureUsage::SampledImage);
    static_assert((t & TextureUsage::StorageImage) == TextureUsage::None);

    constexpr auto s = SwapchainUsage::ColorAttachment | SwapchainUsage::TransferDst;
    static_assert(static_cast<std::uint32_t>(s) ==
                  (static_cast<std::uint32_t>(SwapchainUsage::ColorAttachment) |
                   static_cast<std::uint32_t>(SwapchainUsage::TransferDst)));
}

TEST_CASE("CommandPoolFlagBits: bitwise OR composes; alias name CommandPoolFlags compiles")
{
    constexpr auto flags = CommandPoolFlagBits::Transient | CommandPoolFlagBits::AllowIndividualReset;
    CHECK(static_cast<std::uint32_t>(flags) ==
          (static_cast<std::uint32_t>(CommandPoolFlagBits::Transient) |
           static_cast<std::uint32_t>(CommandPoolFlagBits::AllowIndividualReset)));
    static_assert(std::is_same_v<CommandPoolFlags, CommandPoolFlagBits>);
}

TEST_CASE("PipelineStageMask / AccessMask: bitwise compositions")
{
    constexpr auto stages = PipelineStageMask::PixelShader | PipelineStageMask::Transfer;
    static_assert((stages & PipelineStageMask::PixelShader) == PipelineStageMask::PixelShader);
    static_assert((stages & PipelineStageMask::ComputeShader) == PipelineStageMask::None);

    constexpr auto access = AccessMask::ShaderResourceRead | AccessMask::TransferWrite;
    static_assert((access & AccessMask::ShaderResourceRead) == AccessMask::ShaderResourceRead);
    static_assert((access & AccessMask::ColorAttachmentRead) == AccessMask::None);
}

TEST_CASE("Validation levels: monotonic ordering")
{
    CHECK(RhiValidationLevel::Off      < RhiValidationLevel::Basic);
    CHECK(RhiValidationLevel::Basic    < RhiValidationLevel::Standard);
    CHECK(RhiValidationLevel::Standard < RhiValidationLevel::Strict);

    CHECK(GapiValidationLevel::Off      < GapiValidationLevel::Basic);
    CHECK(GapiValidationLevel::Basic    < GapiValidationLevel::Standard);
    CHECK(GapiValidationLevel::Standard < GapiValidationLevel::Gpu);
    CHECK(GapiValidationLevel::Gpu      < GapiValidationLevel::GpuAggressive);
}

TEST_CASE("DeviceType / QueueType / IndexType / PrimitiveTopology enumerator round-trip")
{
    CHECK(static_cast<std::uint32_t>(DeviceType::Vulkan) == 1u);
    CHECK(static_cast<std::uint32_t>(QueueType::Count) >= 4u);
    CHECK(static_cast<std::uint32_t>(IndexType::Uint16) == 0u);
    CHECK(static_cast<std::uint32_t>(PrimitiveTopology::TriangleList) == 3u);
}

TEST_SUITE_END();
