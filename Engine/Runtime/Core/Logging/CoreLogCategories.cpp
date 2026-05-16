/// @file
/// @brief Storage for the default LogCategory instances declared in CoreLogCategories.h.

#include "Core/Logging/CoreLogCategories.h"

namespace Hylux
{

HYLUX_DEFINE_LOG_CATEGORY(LogCore,   Info, Trace);
HYLUX_DEFINE_LOG_CATEGORY(LogEngine, Info, Trace);
HYLUX_DEFINE_LOG_CATEGORY(LogRender, Info, Trace);
HYLUX_DEFINE_LOG_CATEGORY(LogAsset,  Info, Trace);

} // namespace Hylux
