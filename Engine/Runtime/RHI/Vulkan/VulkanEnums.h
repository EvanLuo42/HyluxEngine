/// @file
/// @brief Mapping helpers between Hylux RHI enums and their Vulkan counterparts.

#pragma once

#include "RHI/IRHIDescriptorSet.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/Vulkan/VulkanCommon.h"

namespace Hylux::RHI::Vulkan
{

[[nodiscard]] VkSampleCountFlagBits ToVkSampleCount(std::uint32_t sampleCount) noexcept;
[[nodiscard]] VkImageType            ToVkImageType(TextureDimension dim) noexcept;
[[nodiscard]] VkImageViewType        ToVkImageViewType(TextureDimension dim) noexcept;
[[nodiscard]] VkPrimitiveTopology    ToVkTopology(PrimitiveTopology topology) noexcept;
[[nodiscard]] VkCompareOp            ToVkCompareOp(CompareOp op) noexcept;
[[nodiscard]] VkBlendFactor          ToVkBlendFactor(BlendFactor f) noexcept;
[[nodiscard]] VkBlendOp              ToVkBlendOp(BlendOp op) noexcept;
[[nodiscard]] VkStencilOp            ToVkStencilOp(StencilOp op) noexcept;
[[nodiscard]] VkCullModeFlags        ToVkCullMode(CullMode mode) noexcept;
[[nodiscard]] VkFrontFace            ToVkFrontFace(FrontFace face) noexcept;
[[nodiscard]] VkPolygonMode          ToVkPolygonMode(FillMode mode) noexcept;
[[nodiscard]] VkFilter               ToVkFilter(FilterMode mode) noexcept;
[[nodiscard]] VkSamplerMipmapMode    ToVkMipmapMode(MipFilterMode mode) noexcept;
[[nodiscard]] VkSamplerAddressMode   ToVkAddressMode(AddressMode mode) noexcept;
[[nodiscard]] VkAttachmentLoadOp     ToVkLoadOp(LoadOp op) noexcept;
[[nodiscard]] VkAttachmentStoreOp    ToVkStoreOp(StoreOp op) noexcept;
[[nodiscard]] VkImageLayout          ToVkImageLayout(ImageLayout layout) noexcept;
[[nodiscard]] VkBufferUsageFlags     ToVkBufferUsage(BufferUsage usage) noexcept;
[[nodiscard]] VkImageUsageFlags      ToVkImageUsage(TextureUsage usage) noexcept;
[[nodiscard]] VkImageUsageFlags      ToVkSwapchainUsage(SwapchainUsage usage) noexcept;
[[nodiscard]] VkPresentModeKHR       ToVkPresentMode(PresentMode mode) noexcept;
[[nodiscard]] VkIndexType            ToVkIndexType(IndexType type) noexcept;
[[nodiscard]] VkShaderStageFlags     ToVkShaderStageFlags(ShaderStage stages) noexcept;
[[nodiscard]] VkShaderStageFlagBits  ToVkShaderStageBit(ShaderStage stage) noexcept;
[[nodiscard]] VkDescriptorType       ToVkDescriptorType(DescriptorType type) noexcept;
[[nodiscard]] VkQueryType            ToVkQueryType(QueryType type) noexcept;

[[nodiscard]] VkPipelineStageFlags2  ToVkPipelineStages(PipelineStageMask mask) noexcept;
[[nodiscard]] VkAccessFlags2         ToVkAccessFlags(AccessMask mask) noexcept;

} // namespace Hylux::RHI::Vulkan
