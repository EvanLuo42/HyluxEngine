/// @file
/// @brief Format mapping tables between Hylux::RHI::Format and VkFormat.

#include "RHI/Vulkan/VulkanFormat.h"

namespace Hylux::RHI::Vulkan
{

VkFormat ToVkFormat(Format format) noexcept
{
    switch (format)
    {
        case Format::Unknown:           return VK_FORMAT_UNDEFINED;

        case Format::R8Unorm:           return VK_FORMAT_R8_UNORM;
        case Format::R8Snorm:           return VK_FORMAT_R8_SNORM;
        case Format::R8Uint:            return VK_FORMAT_R8_UINT;
        case Format::R8Sint:            return VK_FORMAT_R8_SINT;

        case Format::R16Unorm:          return VK_FORMAT_R16_UNORM;
        case Format::R16Snorm:          return VK_FORMAT_R16_SNORM;
        case Format::R16Uint:           return VK_FORMAT_R16_UINT;
        case Format::R16Sint:           return VK_FORMAT_R16_SINT;
        case Format::R16Float:          return VK_FORMAT_R16_SFLOAT;

        case Format::R32Uint:           return VK_FORMAT_R32_UINT;
        case Format::R32Sint:           return VK_FORMAT_R32_SINT;
        case Format::R32Float:          return VK_FORMAT_R32_SFLOAT;

        case Format::RG8Unorm:          return VK_FORMAT_R8G8_UNORM;
        case Format::RG8Snorm:          return VK_FORMAT_R8G8_SNORM;
        case Format::RG8Uint:           return VK_FORMAT_R8G8_UINT;
        case Format::RG8Sint:           return VK_FORMAT_R8G8_SINT;

        case Format::RG16Unorm:         return VK_FORMAT_R16G16_UNORM;
        case Format::RG16Snorm:         return VK_FORMAT_R16G16_SNORM;
        case Format::RG16Uint:          return VK_FORMAT_R16G16_UINT;
        case Format::RG16Sint:          return VK_FORMAT_R16G16_SINT;
        case Format::RG16Float:         return VK_FORMAT_R16G16_SFLOAT;

        case Format::RG32Uint:          return VK_FORMAT_R32G32_UINT;
        case Format::RG32Sint:          return VK_FORMAT_R32G32_SINT;
        case Format::RG32Float:         return VK_FORMAT_R32G32_SFLOAT;

        case Format::RGB32Uint:         return VK_FORMAT_R32G32B32_UINT;
        case Format::RGB32Sint:         return VK_FORMAT_R32G32B32_SINT;
        case Format::RGB32Float:        return VK_FORMAT_R32G32B32_SFLOAT;

        case Format::RGBA8Unorm:        return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8UnormSrgb:    return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::RGBA8Snorm:        return VK_FORMAT_R8G8B8A8_SNORM;
        case Format::RGBA8Uint:         return VK_FORMAT_R8G8B8A8_UINT;
        case Format::RGBA8Sint:         return VK_FORMAT_R8G8B8A8_SINT;

        case Format::BGRA8Unorm:        return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8UnormSrgb:    return VK_FORMAT_B8G8R8A8_SRGB;

        case Format::RGBA16Unorm:       return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16Snorm:       return VK_FORMAT_R16G16B16A16_SNORM;
        case Format::RGBA16Uint:        return VK_FORMAT_R16G16B16A16_UINT;
        case Format::RGBA16Sint:        return VK_FORMAT_R16G16B16A16_SINT;
        case Format::RGBA16Float:       return VK_FORMAT_R16G16B16A16_SFLOAT;

        case Format::RGBA32Uint:        return VK_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32Sint:        return VK_FORMAT_R32G32B32A32_SINT;
        case Format::RGBA32Float:       return VK_FORMAT_R32G32B32A32_SFLOAT;

        case Format::RGB10A2Unorm:      return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case Format::RGB10A2Uint:       return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case Format::RG11B10Float:      return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::RGB9E5Float:       return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

        case Format::D16Unorm:          return VK_FORMAT_D16_UNORM;
        case Format::D24UnormS8Uint:    return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32Float:          return VK_FORMAT_D32_SFLOAT;
        case Format::D32FloatS8Uint:    return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case Format::BC1RgbaUnorm:      return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case Format::BC1RgbaUnormSrgb:  return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case Format::BC2Unorm:          return VK_FORMAT_BC2_UNORM_BLOCK;
        case Format::BC2UnormSrgb:      return VK_FORMAT_BC2_SRGB_BLOCK;
        case Format::BC3Unorm:          return VK_FORMAT_BC3_UNORM_BLOCK;
        case Format::BC3UnormSrgb:      return VK_FORMAT_BC3_SRGB_BLOCK;
        case Format::BC4Unorm:          return VK_FORMAT_BC4_UNORM_BLOCK;
        case Format::BC4Snorm:          return VK_FORMAT_BC4_SNORM_BLOCK;
        case Format::BC5Unorm:          return VK_FORMAT_BC5_UNORM_BLOCK;
        case Format::BC5Snorm:          return VK_FORMAT_BC5_SNORM_BLOCK;
        case Format::BC6HUfloat:        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case Format::BC6HSfloat:        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case Format::BC7Unorm:          return VK_FORMAT_BC7_UNORM_BLOCK;
        case Format::BC7UnormSrgb:      return VK_FORMAT_BC7_SRGB_BLOCK;

        case Format::Etc2Rgb8Unorm:        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case Format::Etc2Rgb8UnormSrgb:    return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        case Format::Etc2Rgb8A1Unorm:      return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case Format::Etc2Rgb8A1UnormSrgb:  return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        case Format::Etc2RgbA8Unorm:       return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case Format::Etc2RgbA8UnormSrgb:   return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        case Format::Eac11Unorm:           return VK_FORMAT_EAC_R11_UNORM_BLOCK;
        case Format::Eac11Snorm:           return VK_FORMAT_EAC_R11_SNORM_BLOCK;
        case Format::Eac22Unorm:           return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        case Format::Eac22Snorm:           return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

        case Format::Astc4x4Unorm:         return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case Format::Astc4x4UnormSrgb:     return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case Format::Astc5x4Unorm:         return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case Format::Astc5x4UnormSrgb:     return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case Format::Astc5x5Unorm:         return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case Format::Astc5x5UnormSrgb:     return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case Format::Astc6x5Unorm:         return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case Format::Astc6x5UnormSrgb:     return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case Format::Astc6x6Unorm:         return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case Format::Astc6x6UnormSrgb:     return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case Format::Astc8x5Unorm:         return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case Format::Astc8x5UnormSrgb:     return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case Format::Astc8x6Unorm:         return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case Format::Astc8x6UnormSrgb:     return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case Format::Astc8x8Unorm:         return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case Format::Astc8x8UnormSrgb:     return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case Format::Astc10x5Unorm:        return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case Format::Astc10x5UnormSrgb:    return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case Format::Astc10x6Unorm:        return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case Format::Astc10x6UnormSrgb:    return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case Format::Astc10x8Unorm:        return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case Format::Astc10x8UnormSrgb:    return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case Format::Astc10x10Unorm:       return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case Format::Astc10x10UnormSrgb:   return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case Format::Astc12x10Unorm:       return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case Format::Astc12x10UnormSrgb:   return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case Format::Astc12x12Unorm:       return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case Format::Astc12x12UnormSrgb:   return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;

        case Format::Count:                return VK_FORMAT_UNDEFINED;
    }
    return VK_FORMAT_UNDEFINED;
}

Format FromVkFormat(VkFormat format) noexcept
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM:                return Format::R8Unorm;
        case VK_FORMAT_R8_SNORM:                return Format::R8Snorm;
        case VK_FORMAT_R8_UINT:                 return Format::R8Uint;
        case VK_FORMAT_R8_SINT:                 return Format::R8Sint;
        case VK_FORMAT_R16_UNORM:               return Format::R16Unorm;
        case VK_FORMAT_R16_SNORM:               return Format::R16Snorm;
        case VK_FORMAT_R16_UINT:                return Format::R16Uint;
        case VK_FORMAT_R16_SINT:                return Format::R16Sint;
        case VK_FORMAT_R16_SFLOAT:              return Format::R16Float;
        case VK_FORMAT_R32_UINT:                return Format::R32Uint;
        case VK_FORMAT_R32_SINT:                return Format::R32Sint;
        case VK_FORMAT_R32_SFLOAT:              return Format::R32Float;
        case VK_FORMAT_R8G8B8A8_UNORM:          return Format::RGBA8Unorm;
        case VK_FORMAT_R8G8B8A8_SRGB:           return Format::RGBA8UnormSrgb;
        case VK_FORMAT_B8G8R8A8_UNORM:          return Format::BGRA8Unorm;
        case VK_FORMAT_B8G8R8A8_SRGB:           return Format::BGRA8UnormSrgb;
        case VK_FORMAT_R16G16B16A16_SFLOAT:     return Format::RGBA16Float;
        case VK_FORMAT_R32G32B32A32_SFLOAT:     return Format::RGBA32Float;
        case VK_FORMAT_D16_UNORM:               return Format::D16Unorm;
        case VK_FORMAT_D24_UNORM_S8_UINT:       return Format::D24UnormS8Uint;
        case VK_FORMAT_D32_SFLOAT:              return Format::D32Float;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:      return Format::D32FloatS8Uint;
        default:                                return Format::Unknown;
    }
}

VkImageAspectFlags FormatAspect(Format format) noexcept
{
    VkImageAspectFlags aspects = 0;
    if (FormatHasDepth(format))   aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (FormatHasStencil(format)) aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
    if (aspects == 0)             aspects  = VK_IMAGE_ASPECT_COLOR_BIT;
    return aspects;
}

} // namespace Hylux::RHI::Vulkan
