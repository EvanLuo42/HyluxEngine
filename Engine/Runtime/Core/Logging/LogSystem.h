/// @file
/// @brief Engine subsystem that constructs the logging pipeline and swaps in the real dispatcher.

#pragma once

#include "Core/Logging/ILogSink.h"
#include "Core/Logging/LogDispatcher.h"
#include "Engine/ISubsystem.h"

#include <memory>
#include <string>
#include <vector>

namespace Hylux
{

/// @brief Construction-time configuration for LogSystem.
struct LogSystemConfig
{
    bool                                   async          = false;
    bool                                   enableConsole  = true;
    bool                                   enableFile     = true;
    bool                                   enableDebugger = true;
    std::string                            logDirectory   = "Logs";
    std::vector<std::unique_ptr<ILogSink>> extraSinks;
};

/// @brief Owns the active LogDispatcher and its sinks. Before Initialize and after Shutdown,
///        the built-in bootstrap dispatcher (stderr) handles all log calls so the engine is
///        never in a state where logging would crash.
class LogSystem final : public ISubsystem
{
public:
    explicit LogSystem(LogSystemConfig config = {});
    ~LogSystem() override = default;
    LogSystem(const LogSystem&) = delete;
    LogSystem& operator=(const LogSystem&) = delete;

    void Initialize(Engine& engine) override;
    void Shutdown() override;

    [[nodiscard]] const LogSystemConfig& Config() const noexcept { return config_; }

private:
    LogSystemConfig                config_;
    std::unique_ptr<LogDispatcher> dispatcher_;
};

} // namespace Hylux
