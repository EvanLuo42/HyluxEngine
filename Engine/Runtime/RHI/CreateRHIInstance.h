/// @file
/// @brief Free function that bootstraps an IRHIInstance. The definition lives in the
///        selected backend translation unit; only the declaration is shipped with the API.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIInstance.h"
#include "RHI/RHIInstanceDesc.h"

namespace Hylux::RHI
{

/// @brief Creates an IRHIInstance for the backend named in instanceDesc.preferredDevice.
///        Loads the requested capture tool when validation is enabled, registers debug
///        callbacks to LogRender, and enumerates adapters. Returns nullptr on failure;
///        a Fatal log record describes the cause.
[[nodiscard]] Ref<IRHIInstance> CreateRHIInstance(const InstanceDesc& instanceDesc);

} // namespace Hylux::RHI
