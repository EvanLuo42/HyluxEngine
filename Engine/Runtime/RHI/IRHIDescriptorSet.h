/// @file
/// @brief Classic descriptor set interface. Reserved for the edge-case binding path; most
///        rendering goes through push constants + bindless heaps and does not need these.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <span>

namespace Hylux::RHI
{

/// @brief Descriptor binding kind within an explicit descriptor set layout.
enum class DescriptorType : std::uint32_t
{
    UniformBuffer = 0,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic,
    SampledImage,
    StorageImage,
    Sampler,
    CombinedImageSampler,
    InputAttachment,
    AccelerationStructure,
};

/// @brief Single binding within a DescriptorSetLayoutDesc.
struct DescriptorSetLayoutBinding
{
    std::uint32_t  binding{0};
    DescriptorType type{DescriptorType::UniformBuffer};
    std::uint32_t  count{1};
    ShaderStage    stages{ShaderStage::All};
};

/// @brief Layout description for an explicit (non-bindless) descriptor set.
struct DescriptorSetLayoutDesc
{
    std::span<const DescriptorSetLayoutBinding> bindings{};
};

/// @brief Single update entry passed to IRHIDescriptorSet::Update.
struct DescriptorWrite
{
    std::uint32_t  binding{0};
    std::uint32_t  arrayElement{0};
    DescriptorType type{DescriptorType::UniformBuffer};
    IRHIBuffer*    buffer{nullptr};
    std::uint64_t  bufferOffset{0};
    std::uint64_t  bufferSize{0};
    IRHITextureView* textureView{nullptr};
    IRHISampler*     sampler{nullptr};
    IRHIAccelerationStructure* accelerationStructure{nullptr};
};

/// @brief Reference-counted handle to a descriptor set layout.
class IRHIDescriptorSetLayout : public IRHIObject
{
public:
    /// @brief Returns the descriptor the layout was created with.
    [[nodiscard]] virtual const DescriptorSetLayoutDesc& GetDesc() const noexcept = 0;
};

/// @brief Reference-counted handle to a descriptor set allocated from the device's pool.
class IRHIDescriptorSet : public IRHIObject
{
public:
    /// @brief Returns the layout the set was allocated against.
    [[nodiscard]] virtual IRHIDescriptorSetLayout* GetLayout() const noexcept = 0;

    /// @brief Atomically updates the listed bindings.
    virtual void Update(std::span<const DescriptorWrite> writes) = 0;
};

} // namespace Hylux::RHI
