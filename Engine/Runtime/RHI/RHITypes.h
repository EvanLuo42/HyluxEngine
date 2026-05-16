/// @file
/// @brief Common primitive value types used across the RHI public surface.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief 2D unsigned extent in texels or pixels.
struct Extent2D
{
    std::uint32_t width{0};
    std::uint32_t height{0};
};

/// @brief 3D unsigned extent in texels.
struct Extent3D
{
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t depth{1};
};

/// @brief 2D signed offset in texels or pixels.
struct Offset2D
{
    std::int32_t x{0};
    std::int32_t y{0};
};

/// @brief 3D signed offset in texels.
struct Offset3D
{
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
};

/// @brief Axis-aligned 2D rectangle defined by an origin and an extent.
struct Rect2D
{
    Offset2D offset{};
    Extent2D extent{};
};

/// @brief 3D viewport with depth range, matching Vulkan/D3D12 conventions.
struct Viewport
{
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float minDepth{0.0f};
    float maxDepth{1.0f};
};

/// @brief Floating-point RGBA color clear value.
struct ClearColorValue
{
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

/// @brief Combined depth/stencil clear value used by render attachments and barriers.
struct ClearDepthStencilValue
{
    float         depth{1.0f};
    std::uint8_t  stencil{0};
};

/// @brief Either a color or depth/stencil clear value tag selected by aspect.
struct ClearValue
{
    bool                    isDepthStencil{false};
    ClearColorValue         color{};
    ClearDepthStencilValue  depthStencil{};
};

/// @brief Linear range expressed as a (first, count) pair. Used for mip and array slices.
struct SubresourceRange
{
    std::uint32_t baseMipLevel{0};
    std::uint32_t mipLevelCount{1};
    std::uint32_t baseArrayLayer{0};
    std::uint32_t arrayLayerCount{1};
};

/// @brief Sentinel for "all remaining levels" in SubresourceRange::mipLevelCount or arrayLayerCount.
inline constexpr std::uint32_t kRemainingLevels = 0xFFFFFFFFu;

/// @brief Sentinel for "all remaining bytes" in buffer ranges.
inline constexpr std::uint64_t kWholeSize = 0xFFFFFFFFFFFFFFFFull;

/// @brief Maximum number of color attachments addressable in a single dynamic rendering pass.
inline constexpr std::uint32_t kMaxColorAttachments = 8u;

/// @brief Maximum number of bytes the engine reserves for push constants across all shader stages.
///        Vulkan guarantees 128 bytes; D3D12 root sig budget leaves room for 32 DWORDs.
inline constexpr std::uint32_t kMaxPushConstantSize = 128u;

} // namespace Hylux::RHI
