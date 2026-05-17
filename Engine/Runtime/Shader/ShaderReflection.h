/// @file
/// @brief Runtime shader reflection metadata. POD that lives inside a ShaderArchive's
///        reflection pool and is referenced by ShaderRef. The renderer consumes this
///        when building pipeline layouts and validating vertex inputs.

#pragma once

#include "RHI/RHIEnums.h"

#include <cstdint>

namespace Hylux::Shader
{

/// @brief Describes a single descriptor binding declared by a shader (used only by the
///        explicit fallback path; the bindless heap is the default).
struct ReflectionDescriptorBinding
{
    std::uint32_t setIndex{0};
    std::uint32_t binding{0};
    std::uint32_t descriptorType{0};
    std::uint32_t descriptorCount{0};
};

/// @brief Describes a single vertex input attribute consumed by a vertex shader. The
///        renderer validates the runtime VertexInputDesc against this.
struct ReflectionVertexAttribute
{
    std::uint32_t location{0};
    std::uint32_t format{0};
    std::uint32_t binding{0};
    std::uint32_t offset{0};
};

/// @brief Top-level reflection record stored in the archive's reflection pool. Variable-
///        sized arrays (bindings, attributes) immediately follow the record in the same
///        pool; their offsets are byte offsets relative to the start of THIS record.
struct ShaderReflection
{
    std::uint32_t pushConstantSize{0};
    std::uint32_t pushConstantVisibility{0};
    std::uint64_t pipelineLayoutHash{0};
    std::uint32_t descriptorBindingOffset{0};
    std::uint16_t descriptorBindingCount{0};
    std::uint16_t reserved0{0};
    std::uint32_t vertexAttributeOffset{0};
    std::uint16_t vertexAttributeCount{0};
    std::uint16_t reserved1{0};
    std::uint32_t sourceFileStringOffset{0};
    std::uint32_t sourceFileStringSize{0};
};

static_assert(sizeof(ShaderReflection) == 40, "ShaderReflection on-disk layout drift");

} // namespace Hylux::Shader
