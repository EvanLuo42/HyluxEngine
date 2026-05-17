/// @file
/// @brief Factory for the engine-side RHI validation wrapper. Wraps an IRHIDevice and
///        intercepts every call to check engine-level contracts before forwarding to the
///        underlying backend. Independent of the GAPI debug layer toggled by
///        GapiValidationLevel.
///
///        **Status:** the wrapper is a stub. Any non-Off level is treated the same and
///        the factory returns the inner device unchanged after a single Warn log. Real
///        contract checking lands when the wrapper itself is implemented; until then,
///        configure GapiValidationLevel for driver-side validation.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIValidation.h"

namespace Hylux::RHI
{

/// @brief Returns a wrapper IRHIDevice that validates engine contracts at the configured
///        level and delegates to the inner device. The wrapper's GetNativeHandle delegates
///        to the inner device so external tools observe no difference. Pass Off to receive
///        the inner device unchanged (no wrapper allocation). See the file-level note for
///        the wrapper's current stub status.
[[nodiscard]] Ref<IRHIDevice> CreateRhiValidationWrapper(Ref<IRHIDevice> innerDevice,
                                                         RhiValidationLevel level);

} // namespace Hylux::RHI
