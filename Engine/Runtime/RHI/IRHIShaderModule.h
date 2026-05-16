/// @file
/// @brief Shader module interface and bytecode descriptor. The RHI shader module is a thin
///        bytecode container; compilation, reflection, and variant management live in the
///        separate shader subsystem.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"

#include <cstddef>
#include <span>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Caller-owned bytecode bundle handed to IRHIDevice::CreateShaderModule. The spans
///        and string_view must remain valid only for the duration of the create call;
///        backends copy what they need.
struct ShaderBytecode
{
    ShaderBytecodeFormat       format{ShaderBytecodeFormat::Spirv};
    ShaderStage                stage{ShaderStage::None};
    std::string_view           entryPoint{};
    std::span<const std::byte> data{};
};

/// @brief Reference-counted handle to a compiled shader module owned by the backend.
class IRHIShaderModule : public IRHIObject
{
public:
    /// @brief Returns the bytecode encoding the module was created from.
    [[nodiscard]] virtual ShaderBytecodeFormat GetFormat() const noexcept = 0;

    /// @brief Returns the shader stage the module targets.
    [[nodiscard]] virtual ShaderStage GetStage() const noexcept = 0;

    /// @brief Returns the entry point function name.
    [[nodiscard]] virtual std::string_view GetEntryPoint() const noexcept = 0;

    /// @brief Returns a view over the bytecode storage retained by the backend.
    [[nodiscard]] virtual std::span<const std::byte> GetBytecode() const noexcept = 0;
};

} // namespace Hylux::RHI
