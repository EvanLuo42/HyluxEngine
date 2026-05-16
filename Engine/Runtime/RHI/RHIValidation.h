/// @file
/// @brief Independent validation level controls for the engine-side RHI wrapper and the
///        underlying graphics API debug layer.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Engine-side validation level. Selects how much contract checking the
///        RhiValidationLayer wrapper performs against IRHIDevice / IRHICommandList calls.
enum class RhiValidationLevel : std::uint32_t
{
    Off = 0,
    Basic,
    Standard,
    Strict,
};

/// @brief Underlying GAPI validation level. Selects how the backend configures its native
///        debug layer (Vulkan validation layers, D3D12 debug layer, GPU-based validation).
enum class GapiValidationLevel : std::uint32_t
{
    Off = 0,
    Basic,
    Standard,
    Gpu,
    GpuAggressive,
};

} // namespace Hylux::RHI
