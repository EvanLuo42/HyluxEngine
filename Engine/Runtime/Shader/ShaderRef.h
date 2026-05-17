/// @file
/// @brief Lightweight non-owning handle to a resolved shader entry. Returned by the
///        ShaderSubsystem's lookup methods. The module pointer's lifetime is governed by
///        ShaderModuleCache; the reflection pointer's lifetime is governed by the open
///        ShaderArchive's mapped blob.

#pragma once

#include "RHI/RHIForward.h"
#include "Shader/ShaderId.h"
#include "Shader/ShaderReflection.h"

namespace Hylux::Shader
{

/// @brief Non-owning view onto a successfully resolved shader. Empty (operator bool ==
///        false) when the lookup failed. Holders must not outlive the ShaderSubsystem
///        that produced them, and they must observe archive reloads (handles obtained
///        before a reload are not refreshed automatically).
struct ShaderRef
{
    RHI::IRHIShaderModule*  module{nullptr};
    const ShaderReflection* reflection{nullptr};
    ShaderId                id{};

    [[nodiscard]] explicit operator bool() const noexcept { return module != nullptr; }
};

} // namespace Hylux::Shader
