/// @file
/// @brief Debugger sink: routes lines to OutputDebugStringW on Windows when a debugger is attached.

#pragma once

#include "Core/Logging/ILogSink.h"

namespace Hylux
{

/// @brief On Windows, writes formatted lines to OutputDebugStringW when IsDebuggerPresent().
///        On other platforms this is a stateless no-op kept for API parity.
class DebuggerSink final : public ILogSink
{
public:
    DebuggerSink() = default;
    ~DebuggerSink() override = default;
    DebuggerSink(const DebuggerSink&) = delete;
    DebuggerSink& operator=(const DebuggerSink&) = delete;

    void Submit(const LogRecord& record) override;
    void Flush() override;
};

} // namespace Hylux
