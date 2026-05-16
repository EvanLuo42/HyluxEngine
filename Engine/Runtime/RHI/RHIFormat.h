/// @file
/// @brief Format enumeration spanning DXGI / Vulkan / Metal / console superset.
///        Values are stable identifiers, not raw API constants.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Texel and vertex element format identifiers. Backends translate to native enums.
enum class Format : std::uint32_t
{
    Unknown = 0,

    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,

    R16Unorm,
    R16Snorm,
    R16Uint,
    R16Sint,
    R16Float,

    R32Uint,
    R32Sint,
    R32Float,

    RG8Unorm,
    RG8Snorm,
    RG8Uint,
    RG8Sint,

    RG16Unorm,
    RG16Snorm,
    RG16Uint,
    RG16Sint,
    RG16Float,

    RG32Uint,
    RG32Sint,
    RG32Float,

    RGB32Uint,
    RGB32Sint,
    RGB32Float,

    RGBA8Unorm,
    RGBA8UnormSrgb,
    RGBA8Snorm,
    RGBA8Uint,
    RGBA8Sint,

    BGRA8Unorm,
    BGRA8UnormSrgb,

    RGBA16Unorm,
    RGBA16Snorm,
    RGBA16Uint,
    RGBA16Sint,
    RGBA16Float,

    RGBA32Uint,
    RGBA32Sint,
    RGBA32Float,

    RGB10A2Unorm,
    RGB10A2Uint,
    RG11B10Float,
    RGB9E5Float,

    D16Unorm,
    D24UnormS8Uint,
    D32Float,
    D32FloatS8Uint,

    BC1RgbaUnorm,
    BC1RgbaUnormSrgb,
    BC2Unorm,
    BC2UnormSrgb,
    BC3Unorm,
    BC3UnormSrgb,
    BC4Unorm,
    BC4Snorm,
    BC5Unorm,
    BC5Snorm,
    BC6HUfloat,
    BC6HSfloat,
    BC7Unorm,
    BC7UnormSrgb,

    Etc2Rgb8Unorm,
    Etc2Rgb8UnormSrgb,
    Etc2Rgb8A1Unorm,
    Etc2Rgb8A1UnormSrgb,
    Etc2RgbA8Unorm,
    Etc2RgbA8UnormSrgb,
    Eac11Unorm,
    Eac11Snorm,
    Eac22Unorm,
    Eac22Snorm,

    Astc4x4Unorm,
    Astc4x4UnormSrgb,
    Astc5x4Unorm,
    Astc5x4UnormSrgb,
    Astc5x5Unorm,
    Astc5x5UnormSrgb,
    Astc6x5Unorm,
    Astc6x5UnormSrgb,
    Astc6x6Unorm,
    Astc6x6UnormSrgb,
    Astc8x5Unorm,
    Astc8x5UnormSrgb,
    Astc8x6Unorm,
    Astc8x6UnormSrgb,
    Astc8x8Unorm,
    Astc8x8UnormSrgb,
    Astc10x5Unorm,
    Astc10x5UnormSrgb,
    Astc10x6Unorm,
    Astc10x6UnormSrgb,
    Astc10x8Unorm,
    Astc10x8UnormSrgb,
    Astc10x10Unorm,
    Astc10x10UnormSrgb,
    Astc12x10Unorm,
    Astc12x10UnormSrgb,
    Astc12x12Unorm,
    Astc12x12UnormSrgb,

    Count,
};

/// @brief Bit flags describing what a backend supports for a given Format on a given device.
struct FormatSupport
{
    std::uint32_t bits{0};

    static constexpr std::uint32_t kSampledImage          = 1u << 0;
    static constexpr std::uint32_t kSampledImageFilterLinear = 1u << 1;
    static constexpr std::uint32_t kStorageImage          = 1u << 2;
    static constexpr std::uint32_t kStorageImageAtomic    = 1u << 3;
    static constexpr std::uint32_t kColorAttachment       = 1u << 4;
    static constexpr std::uint32_t kColorAttachmentBlend  = 1u << 5;
    static constexpr std::uint32_t kDepthStencilAttachment= 1u << 6;
    static constexpr std::uint32_t kVertexBuffer          = 1u << 7;
    static constexpr std::uint32_t kUniformTexelBuffer    = 1u << 8;
    static constexpr std::uint32_t kStorageTexelBuffer    = 1u << 9;
    static constexpr std::uint32_t kStorageTexelBufferAtomic = 1u << 10;
    static constexpr std::uint32_t kBlitSrc               = 1u << 11;
    static constexpr std::uint32_t kBlitDst               = 1u << 12;
    static constexpr std::uint32_t kResolveSrc            = 1u << 13;
    static constexpr std::uint32_t kResolveDst            = 1u << 14;
};

/// @brief Returns true when the format carries a depth component (D16, D24S8, D32F, D32FS8).
[[nodiscard]] constexpr bool FormatHasDepth(Format f) noexcept
{
    return f == Format::D16Unorm || f == Format::D24UnormS8Uint ||
           f == Format::D32Float  || f == Format::D32FloatS8Uint;
}

/// @brief Returns true when the format carries a stencil component (D24S8, D32FS8).
[[nodiscard]] constexpr bool FormatHasStencil(Format f) noexcept
{
    return f == Format::D24UnormS8Uint || f == Format::D32FloatS8Uint;
}

/// @brief Returns true when the format is a block-compressed BC/ETC2/EAC/ASTC variant.
[[nodiscard]] constexpr bool FormatIsCompressed(Format f) noexcept
{
    return f >= Format::BC1RgbaUnorm && f <= Format::Astc12x12UnormSrgb;
}

/// @brief Returns true when the format uses an sRGB color space encoding.
[[nodiscard]] constexpr bool FormatIsSrgb(Format f) noexcept
{
    switch (f)
    {
        case Format::RGBA8UnormSrgb:
        case Format::BGRA8UnormSrgb:
        case Format::BC1RgbaUnormSrgb:
        case Format::BC2UnormSrgb:
        case Format::BC3UnormSrgb:
        case Format::BC7UnormSrgb:
        case Format::Etc2Rgb8UnormSrgb:
        case Format::Etc2Rgb8A1UnormSrgb:
        case Format::Etc2RgbA8UnormSrgb:
        case Format::Astc4x4UnormSrgb:
        case Format::Astc5x4UnormSrgb:
        case Format::Astc5x5UnormSrgb:
        case Format::Astc6x5UnormSrgb:
        case Format::Astc6x6UnormSrgb:
        case Format::Astc8x5UnormSrgb:
        case Format::Astc8x6UnormSrgb:
        case Format::Astc8x8UnormSrgb:
        case Format::Astc10x5UnormSrgb:
        case Format::Astc10x6UnormSrgb:
        case Format::Astc10x8UnormSrgb:
        case Format::Astc10x10UnormSrgb:
        case Format::Astc12x10UnormSrgb:
        case Format::Astc12x12UnormSrgb:
            return true;
        default:
            return false;
    }
}

} // namespace Hylux::RHI
