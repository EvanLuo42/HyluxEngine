/// @file
/// @brief Default log categories used across the runtime core.

#pragma once

#include "Core/Logging/LogCategory.h"

namespace Hylux
{

HYLUX_DECLARE_LOG_CATEGORY_EXTERN(LogCore,     Info, Trace);
HYLUX_DECLARE_LOG_CATEGORY_EXTERN(LogEngine,   Info, Trace);
HYLUX_DECLARE_LOG_CATEGORY_EXTERN(LogRender,   Info, Trace);
HYLUX_DECLARE_LOG_CATEGORY_EXTERN(LogAsset,    Info, Trace);

} // namespace Hylux
