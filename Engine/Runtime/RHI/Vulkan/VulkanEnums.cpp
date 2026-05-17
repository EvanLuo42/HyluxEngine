/// @file
/// @brief Enum mapping implementations.

#include "RHI/Vulkan/VulkanEnums.h"

#include "RHI/IRHIDescriptorSet.h"

namespace Hylux::RHI::Vulkan
{

VkSampleCountFlagBits ToVkSampleCount(std::uint32_t sampleCount) noexcept
{
    switch (sampleCount)
    {
        case 1:  return VK_SAMPLE_COUNT_1_BIT;
        case 2:  return VK_SAMPLE_COUNT_2_BIT;
        case 4:  return VK_SAMPLE_COUNT_4_BIT;
        case 8:  return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
        case 32: return VK_SAMPLE_COUNT_32_BIT;
        case 64: return VK_SAMPLE_COUNT_64_BIT;
        default: return VK_SAMPLE_COUNT_1_BIT;
    }
}

VkImageType ToVkImageType(TextureDimension dim) noexcept
{
    switch (dim)
    {
        case TextureDimension::Tex1D:        return VK_IMAGE_TYPE_1D;
        case TextureDimension::Tex1DArray:   return VK_IMAGE_TYPE_1D;
        case TextureDimension::Tex3D:        return VK_IMAGE_TYPE_3D;
        default:                             return VK_IMAGE_TYPE_2D;
    }
}

VkImageViewType ToVkImageViewType(TextureDimension dim) noexcept
{
    switch (dim)
    {
        case TextureDimension::Tex1D:         return VK_IMAGE_VIEW_TYPE_1D;
        case TextureDimension::Tex1DArray:    return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case TextureDimension::Tex2D:         return VK_IMAGE_VIEW_TYPE_2D;
        case TextureDimension::Tex2DArray:    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case TextureDimension::Tex3D:         return VK_IMAGE_VIEW_TYPE_3D;
        case TextureDimension::TexCube:       return VK_IMAGE_VIEW_TYPE_CUBE;
        case TextureDimension::TexCubeArray:  return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
    return VK_IMAGE_VIEW_TYPE_2D;
}

VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology) noexcept
{
    switch (topology)
    {
        case PrimitiveTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::PatchList:     return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkCompareOp ToVkCompareOp(CompareOp op) noexcept
{
    switch (op)
    {
        case CompareOp::Never:        return VK_COMPARE_OP_NEVER;
        case CompareOp::Less:         return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:        return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:      return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:     return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::Always:       return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_ALWAYS;
}

VkBlendFactor ToVkBlendFactor(BlendFactor f) noexcept
{
    switch (f)
    {
        case BlendFactor::Zero:                 return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:                  return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor:             return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:             return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:     return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:             return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:             return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:     return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor:        return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor:return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::ConstantAlpha:        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha:return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SrcAlphaSaturate:     return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case BlendFactor::Src1Color:            return VK_BLEND_FACTOR_SRC1_COLOR;
        case BlendFactor::OneMinusSrc1Color:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case BlendFactor::Src1Alpha:            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case BlendFactor::OneMinusSrc1Alpha:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    return VK_BLEND_FACTOR_ZERO;
}

VkBlendOp ToVkBlendOp(BlendOp op) noexcept
{
    switch (op)
    {
        case BlendOp::Add:             return VK_BLEND_OP_ADD;
        case BlendOp::Subtract:        return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:             return VK_BLEND_OP_MIN;
        case BlendOp::Max:             return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

VkStencilOp ToVkStencilOp(StencilOp op) noexcept
{
    switch (op)
    {
        case StencilOp::Keep:           return VK_STENCIL_OP_KEEP;
        case StencilOp::Zero:           return VK_STENCIL_OP_ZERO;
        case StencilOp::Replace:        return VK_STENCIL_OP_REPLACE;
        case StencilOp::IncrementClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case StencilOp::DecrementClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case StencilOp::Invert:         return VK_STENCIL_OP_INVERT;
        case StencilOp::IncrementWrap:  return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case StencilOp::DecrementWrap:  return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
    return VK_STENCIL_OP_KEEP;
}

VkCullModeFlags ToVkCullMode(CullMode mode) noexcept
{
    switch (mode)
    {
        case CullMode::None:  return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_NONE;
}

VkFrontFace ToVkFrontFace(FrontFace face) noexcept
{
    return face == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE
                                         : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

VkPolygonMode ToVkPolygonMode(FillMode mode) noexcept
{
    return mode == FillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

VkFilter ToVkFilter(FilterMode mode) noexcept
{
    return mode == FilterMode::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

VkSamplerMipmapMode ToVkMipmapMode(MipFilterMode mode) noexcept
{
    return mode == MipFilterMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST
                                          : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

VkSamplerAddressMode ToVkAddressMode(AddressMode mode) noexcept
{
    switch (mode)
    {
        case AddressMode::Repeat:            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case AddressMode::MirroredRepeat:    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case AddressMode::ClampToEdge:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::ClampToBorder:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case AddressMode::MirrorClampToEdge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkAttachmentLoadOp ToVkLoadOp(LoadOp op) noexcept
{
    switch (op)
    {
        case LoadOp::Load:     return VK_ATTACHMENT_LOAD_OP_LOAD;
        case LoadOp::Clear:    return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case LoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_LOAD;
}

VkAttachmentStoreOp ToVkStoreOp(StoreOp op) noexcept
{
    switch (op)
    {
        case StoreOp::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case StoreOp::Resolve:  return VK_ATTACHMENT_STORE_OP_STORE;
    }
    return VK_ATTACHMENT_STORE_OP_STORE;
}

VkImageLayout ToVkImageLayout(ImageLayout layout) noexcept
{
    switch (layout)
    {
        case ImageLayout::Undefined:              return VK_IMAGE_LAYOUT_UNDEFINED;
        case ImageLayout::General:                return VK_IMAGE_LAYOUT_GENERAL;
        case ImageLayout::ColorAttachment:        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ImageLayout::DepthStencilReadOnly:   return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case ImageLayout::ShaderReadOnly:         return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageLayout::TransferSrc:            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ImageLayout::TransferDst:            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ImageLayout::Present:                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case ImageLayout::RayTracingShared:       return VK_IMAGE_LAYOUT_GENERAL;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage) noexcept
{
    VkBufferUsageFlags out = 0;
    const auto raw = static_cast<std::uint32_t>(usage);
    if (raw & static_cast<std::uint32_t>(BufferUsage::TransferSrc))     out |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::TransferDst))     out |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::UniformBuffer))   out |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::StorageBuffer))   out |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::IndexBuffer))     out |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::VertexBuffer))    out |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::IndirectBuffer))  out |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::ShaderDeviceAddress))
        out |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::AccelerationStructureBuildInput))
        out |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
             | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (raw & static_cast<std::uint32_t>(BufferUsage::AccelerationStructureStorage))
        out |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(BufferUsage::ShaderBindingTable))
        out |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
             | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    return out;
}

VkImageUsageFlags ToVkImageUsage(TextureUsage usage) noexcept
{
    VkImageUsageFlags out = 0;
    const auto raw = static_cast<std::uint32_t>(usage);
    if (raw & static_cast<std::uint32_t>(TextureUsage::TransferSrc))            out |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::TransferDst))            out |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::SampledImage))           out |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::StorageImage))           out |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::ColorAttachment))        out |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::DepthStencilAttachment)) out |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::InputAttachment))        out |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (raw & static_cast<std::uint32_t>(TextureUsage::TransientAttachment))    out |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    return out;
}

VkImageUsageFlags ToVkSwapchainUsage(SwapchainUsage usage) noexcept
{
    VkImageUsageFlags out = 0;
    const auto raw = static_cast<std::uint32_t>(usage);
    if (raw & static_cast<std::uint32_t>(SwapchainUsage::ColorAttachment)) out |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (raw & static_cast<std::uint32_t>(SwapchainUsage::TransferDst))     out |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (raw & static_cast<std::uint32_t>(SwapchainUsage::StorageImage))    out |= VK_IMAGE_USAGE_STORAGE_BIT;
    return out;
}

VkPresentModeKHR ToVkPresentMode(PresentMode mode) noexcept
{
    switch (mode)
    {
        case PresentMode::Immediate:   return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case PresentMode::Fifo:        return VK_PRESENT_MODE_FIFO_KHR;
        case PresentMode::FifoRelaxed: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        case PresentMode::Mailbox:     return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkIndexType ToVkIndexType(IndexType type) noexcept
{
    return type == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

VkShaderStageFlags ToVkShaderStageFlags(ShaderStage stages) noexcept
{
    VkShaderStageFlags out = 0;
    const auto raw = static_cast<std::uint32_t>(stages);
    if (raw & static_cast<std::uint32_t>(ShaderStage::Vertex))        out |= VK_SHADER_STAGE_VERTEX_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Hull))          out |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Domain))        out |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Geometry))      out |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Pixel))         out |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Compute))       out |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Task))          out |= VK_SHADER_STAGE_TASK_BIT_EXT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Mesh))          out |= VK_SHADER_STAGE_MESH_BIT_EXT;
    if (raw & static_cast<std::uint32_t>(ShaderStage::RayGeneration)) out |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(ShaderStage::AnyHit))        out |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(ShaderStage::ClosestHit))    out |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Miss))          out |= VK_SHADER_STAGE_MISS_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Intersection))  out |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    if (raw & static_cast<std::uint32_t>(ShaderStage::Callable))      out |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    return out;
}

VkShaderStageFlagBits ToVkShaderStageBit(ShaderStage stage) noexcept
{
    switch (stage)
    {
        case ShaderStage::Vertex:        return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Hull:          return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::Domain:        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::Geometry:      return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::Pixel:         return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:       return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderStage::Task:          return VK_SHADER_STAGE_TASK_BIT_EXT;
        case ShaderStage::Mesh:          return VK_SHADER_STAGE_MESH_BIT_EXT;
        case ShaderStage::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case ShaderStage::AnyHit:        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case ShaderStage::ClosestHit:    return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case ShaderStage::Miss:          return VK_SHADER_STAGE_MISS_BIT_KHR;
        case ShaderStage::Intersection:  return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case ShaderStage::Callable:      return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        default:                         return VK_SHADER_STAGE_ALL;
    }
}

VkDescriptorType ToVkDescriptorType(DescriptorType type) noexcept
{
    switch (type)
    {
        case DescriptorType::UniformBuffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::StorageBuffer:         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::UniformBufferDynamic:  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case DescriptorType::StorageBufferDynamic:  return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case DescriptorType::SampledImage:          return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::StorageImage:          return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::Sampler:               return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::CombinedImageSampler:  return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::InputAttachment:       return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case DescriptorType::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

VkQueryType ToVkQueryType(QueryType type) noexcept
{
    switch (type)
    {
        case QueryType::Timestamp:           return VK_QUERY_TYPE_TIMESTAMP;
        case QueryType::Occlusion:           return VK_QUERY_TYPE_OCCLUSION;
        case QueryType::OcclusionPrecise:    return VK_QUERY_TYPE_OCCLUSION;
        case QueryType::PipelineStatistics:  return VK_QUERY_TYPE_PIPELINE_STATISTICS;
        case QueryType::AccelerationStructureCompactedSize:
            return VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        case QueryType::AccelerationStructureSerializationSize:
            return VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR;
    }
    return VK_QUERY_TYPE_TIMESTAMP;
}

VkPipelineStageFlags2 ToVkPipelineStages(PipelineStageMask mask) noexcept
{
    VkPipelineStageFlags2 out = 0;
    const auto raw = static_cast<std::uint64_t>(mask);
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::DrawIndirect))         out |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::VertexInput))          out |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::VertexShader))         out |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::HullShader))           out |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::DomainShader))         out |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::GeometryShader))       out |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::PixelShader))          out |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::EarlyFragmentTests))   out |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::LateFragmentTests))    out |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::ColorAttachmentOutput))out |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::ComputeShader))        out |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::TaskShader))           out |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::MeshShader))           out |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::RayTracingShader))     out |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::AccelerationStructureBuild))
        out |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::AccelerationStructureCopy))
        out |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::Transfer))             out |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::Resolve))              out |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::Blit))                 out |= VK_PIPELINE_STAGE_2_BLIT_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::Clear))                out |= VK_PIPELINE_STAGE_2_CLEAR_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::Host))                 out |= VK_PIPELINE_STAGE_2_HOST_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::AllGraphics))          out |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::AllCommands))          out |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::TopOfPipe))            out |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    if (raw & static_cast<std::uint64_t>(PipelineStageMask::BottomOfPipe))         out |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    return out;
}

VkAccessFlags2 ToVkAccessFlags(AccessMask mask) noexcept
{
    VkAccessFlags2 out = 0;
    const auto raw = static_cast<std::uint64_t>(mask);
    if (raw & static_cast<std::uint64_t>(AccessMask::IndirectArgRead))             out |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::IndexBufferRead))             out |= VK_ACCESS_2_INDEX_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::VertexBufferRead))            out |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::UniformRead))                 out |= VK_ACCESS_2_UNIFORM_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::ShaderResourceRead))          out |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::ShaderResourceWrite))         out |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::ColorAttachmentRead))         out |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::ColorAttachmentWrite))        out |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::DepthStencilAttachmentRead))  out |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::DepthStencilAttachmentWrite)) out |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::TransferRead))                out |= VK_ACCESS_2_TRANSFER_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::TransferWrite))               out |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::HostRead))                    out |= VK_ACCESS_2_HOST_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::HostWrite))                   out |= VK_ACCESS_2_HOST_WRITE_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::AccelerationStructureRead))   out |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(AccessMask::AccelerationStructureWrite))  out |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(AccessMask::ShadingRateRead))             out |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
    if (raw & static_cast<std::uint64_t>(AccessMask::MemoryRead))                  out |= VK_ACCESS_2_MEMORY_READ_BIT;
    if (raw & static_cast<std::uint64_t>(AccessMask::MemoryWrite))                 out |= VK_ACCESS_2_MEMORY_WRITE_BIT;
    return out;
}

} // namespace Hylux::RHI::Vulkan
