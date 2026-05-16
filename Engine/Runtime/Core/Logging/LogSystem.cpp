/// @file
/// @brief LogSystem implementation: builds the configured dispatcher, swaps it in atomically.

#include "Core/Logging/LogSystem.h"

#include "Core/Logging/AsyncDispatcher.h"
#include "Core/Logging/Logger.h"
#include "Core/Logging/Sinks/ConsoleSink.h"
#include "Core/Logging/Sinks/DebuggerSink.h"
#include "Core/Logging/Sinks/FileSink.h"
#include "Core/Logging/SyncDispatcher.h"

#include <filesystem>
#include <memory>
#include <utility>

namespace Hylux
{
namespace
{

template <typename DispatcherT>
void AttachSinks(DispatcherT& dispatcher, const LogSystemConfig& config)
{
    if (config.enableConsole)
    {
        dispatcher.AddSink(std::make_unique<ConsoleSink>());
    }
#if defined(HYLUX_DESKTOP)
    if (config.enableFile)
    {
        const std::filesystem::path dir =
            std::filesystem::path(config.logDirectory).is_absolute()
                ? std::filesystem::path(config.logDirectory)
                : std::filesystem::current_path() / config.logDirectory;
        dispatcher.AddSink(std::make_unique<FileSink>(dir));
    }
#else
    (void)config.enableFile;
#endif
#if defined(HYLUX_WINDOWS)
    if (config.enableDebugger)
    {
        dispatcher.AddSink(std::make_unique<DebuggerSink>());
    }
#else
    (void)config.enableDebugger;
#endif
}

} // namespace

LogSystem::LogSystem(LogSystemConfig config)
    : config_(std::move(config))
{
}

void LogSystem::Initialize(Engine& /*engine*/)
{
    if (config_.async)
    {
        auto async = std::make_unique<AsyncDispatcher>();
        AttachSinks(*async, config_);
        async->Start();
        LogDispatcher* raw = async.get();
        dispatcher_ = std::move(async);
        Logging::SetActiveDispatcher(raw);
    }
    else
    {
        auto sync = std::make_unique<SyncDispatcher>();
        AttachSinks(*sync, config_);
        LogDispatcher* raw = sync.get();
        dispatcher_ = std::move(sync);
        Logging::SetActiveDispatcher(raw);
    }
}

void LogSystem::Shutdown()
{
    Logging::RestoreBootstrapDispatcher();
    if (dispatcher_)
    {
        dispatcher_->Flush();
        dispatcher_.reset();
    }
}

} // namespace Hylux
