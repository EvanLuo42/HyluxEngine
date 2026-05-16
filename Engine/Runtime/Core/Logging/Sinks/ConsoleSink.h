/// @file
/// @brief Console sink: writes to stdout (Info/below) and stderr (Warn/above) with optional ANSI color.

#pragma once

#include "Core/Logging/ILogSink.h"

#include <mutex>

namespace Hylux
{

/// @brief Writes log lines to the platform standard streams. Levels at or above Warn go to stderr,
///        the rest go to stdout. ANSI escape sequences are emitted when the underlying device
///        supports them (TTY on POSIX, virtual terminal mode on Windows 10+).
class ConsoleSink final : public ILogSink
{
public:
    ConsoleSink();
    ~ConsoleSink() override = default;
    ConsoleSink(const ConsoleSink&) = delete;
    ConsoleSink& operator=(const ConsoleSink&) = delete;

    void Submit(const LogRecord& record) override;
    void Flush() override;

private:
    std::mutex mutex_;
    bool       colorsEnabled_{false};
};

} // namespace Hylux
