/// @file
/// @brief Log severity levels and string conversion.

#pragma once

#include <cstdint>
#include <string_view>

namespace Hylux
{

/// @brief Severity level for log records. Ordered ascending; Off disables a category.
enum class LogLevel : std::uint8_t
{
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6
};

/// @brief Returns a fixed-width 5-character label for the level, suitable for log line prefixes.
[[nodiscard]] constexpr std::string_view LevelToString(LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF  ";
    }
    return "?????";
}

} // namespace Hylux
