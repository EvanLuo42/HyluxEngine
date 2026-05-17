/// @file
/// @brief Placeholder for the engine-side RHI validation wrapper. The real wrapper will
///        intercept every IRHIDevice call to enforce engine contracts; until that work
///        lands, this implementation simply hands the inner device back unchanged so
///        callers compiled against the header continue to link.

#include "RHI/Validation/RhiValidationLayer.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"

namespace Hylux::RHI
{

Ref<IRHIDevice> CreateRhiValidationWrapper(Ref<IRHIDevice> innerDevice, RhiValidationLevel level)
{
    if (level != RhiValidationLevel::Off)
    {
        HYLUX_LOG(LogRender, Warn,
                  "CreateRhiValidationWrapper: RhiValidationLevel={} requested but the "
                  "engine-side wrapper is a stub; returning the unwrapped device. "
                  "Configure GapiValidationLevel for driver-side validation in the meantime.",
                  static_cast<std::uint32_t>(level));
    }
    return innerDevice;
}

} // namespace Hylux::RHI
