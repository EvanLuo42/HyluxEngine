/// @file
/// @brief Immutable log record passed from dispatcher to sinks.

#pragma once

#include "Core/Logging/LogLevel.h"

#include <chrono>
#include <cstdint>
#include <string_view>
#include <thread>

namespace Hylux
{

/// @brief Source location of a log call site. Pointers reference string literals owned by the binary.
struct LogSite
{
    const char*  file;
    int          line;
    const char*  function;
};

/// @brief A single formatted log entry. message is non-owning; backing storage lives in the dispatcher
///        or thread-local scratch buffer that produced it and must outlive every Submit call.
struct LogRecord
{
    std::chrono::system_clock::time_point timestamp;
    std::thread::id                       threadId;
    LogLevel                              level;
    const char*                           categoryName;
    LogSite                               site;
    std::string_view                      message;
};

} // namespace Hylux
