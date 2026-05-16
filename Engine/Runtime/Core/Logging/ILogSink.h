/// @file
/// @brief Abstract sink interface; concrete sinks own their own thread-safety.

#pragma once

#include "Core/Logging/LogRecord.h"

namespace Hylux
{

/// @brief Output target for log records. Implementations must serialize concurrent Submit calls
///        internally. The LogRecord and the std::string_view inside it are valid only for the
///        duration of the Submit call.
class ILogSink
{
public:
    virtual ~ILogSink() = default;

    /// @brief Writes one record to the sink. May be called concurrently from any thread.
    virtual void Submit(const LogRecord& record) = 0;

    /// @brief Flushes any buffered output to the underlying device.
    virtual void Flush() = 0;
};

} // namespace Hylux
