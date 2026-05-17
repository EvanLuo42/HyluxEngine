/// @file
/// @brief Independent validation level controls for the engine-side RHI wrapper and the
///        underlying graphics API debug layer.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Engine-side validation level. Selects how much contract checking the
///        RhiValidationLayer wrapper performs against IRHIDevice / IRHICommandList calls.
///
/// **Status:** the wrapper itself is currently a stub — every non-Off level is treated
/// the same and the wrapper simply forwards to the inner device, logging a one-time Warn
/// at construction. The enum is preserved so descriptor structs and config files stay
/// stable while the wrapper is being built out. For real validation today use
/// GapiValidationLevel, which drives the backend's native debug layer (Vulkan VL etc.).
enum class RhiValidationLevel : std::uint32_t
{
    Off = 0,
    Basic,
    Standard,
    Strict,
};

/// @brief Underlying GAPI validation level. Selects how the backend configures its native
///        debug layer (Vulkan validation layers, D3D12 debug layer, GPU-based validation).
///        This path is implemented; configure it for real driver-side checks.
enum class GapiValidationLevel : std::uint32_t
{
    Off = 0,
    Basic,
    Standard,
    Gpu,
    GpuAggressive,
};

} // namespace Hylux::RHI
