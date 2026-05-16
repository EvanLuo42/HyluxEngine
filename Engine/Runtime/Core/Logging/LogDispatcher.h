/// @file
/// @brief Dispatcher interface routing log records to one or more sinks.

#pragma once

#include "Core/Logging/LogRecord.h"

namespace Hylux
{

/// @brief Routes records to attached sinks. Concrete dispatchers decide sync vs async delivery.
///        Dispatch must be safe to call from any thread without external synchronization.
class LogDispatcher
{
public:
    virtual ~LogDispatcher() = default;

    /// @brief Delivers a record to all attached sinks. Thread-safe.
    virtual void Dispatch(const LogRecord& record) = 0;

    /// @brief Blocks until every record submitted before this call has been written.
    virtual void Flush() = 0;
};

} // namespace Hylux
