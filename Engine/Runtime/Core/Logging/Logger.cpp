/// @file
/// @brief Logger implementation: bootstrap dispatcher, active-dispatcher pointer, fatal handler.

#include "Core/Logging/Logger.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <utility>

namespace Hylux
{
namespace
{

/// @brief Minimal always-available dispatcher that writes to stderr under a mutex.
///        Used before LogSystem::Initialize and after LogSystem::Shutdown so that log calls
///        from static initializers, subsystem constructors, and destructors never crash.
class BootstrapDispatcher final : public LogDispatcher
{
public:
    void Dispatch(const LogRecord& record) override
    {
        const std::string_view levelLabel = LevelToString(record.level);
        std::lock_guard<std::mutex> guard(mutex_);
        std::fprintf(stderr, "[%.*s] [%s] %.*s\n",
            static_cast<int>(levelLabel.size()), levelLabel.data(),
            record.categoryName,
            static_cast<int>(record.message.size()), record.message.data());
        std::fflush(stderr);
    }

    void Flush() override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        std::fflush(stderr);
    }

private:
    std::mutex mutex_;
};

BootstrapDispatcher       g_bootstrapDispatcher;
std::atomic<LogDispatcher*> g_activeDispatcher{&g_bootstrapDispatcher};

std::mutex                 g_fatalHandlerMutex;
Logging::FatalHandler      g_fatalHandler;

[[noreturn]] void DefaultFatalHandler(const LogRecord&)
{
    if (auto* dispatcher = g_activeDispatcher.load(std::memory_order_acquire))
    {
        dispatcher->Flush();
    }
    std::abort();
}

} // namespace

namespace Logging
{

void SetFatalHandler(FatalHandler handler)
{
    std::lock_guard<std::mutex> guard(g_fatalHandlerMutex);
    g_fatalHandler = std::move(handler);
}

LogDispatcher* SetActiveDispatcher(LogDispatcher* dispatcher) noexcept
{
    LogDispatcher* replacement = dispatcher ? dispatcher : &g_bootstrapDispatcher;
    return g_activeDispatcher.exchange(replacement, std::memory_order_acq_rel);
}

LogDispatcher* GetActiveDispatcher() noexcept
{
    return g_activeDispatcher.load(std::memory_order_acquire);
}

LogDispatcher* RestoreBootstrapDispatcher() noexcept
{
    return g_activeDispatcher.exchange(&g_bootstrapDispatcher, std::memory_order_acq_rel);
}

} // namespace Logging

namespace Detail
{

std::string& LogFormatBuffer() noexcept
{
    thread_local std::string buffer;
    return buffer;
}

void InvokeFatalHandler(const LogRecord& record)
{
    Logging::FatalHandler handler;
    {
        std::lock_guard<std::mutex> guard(g_fatalHandlerMutex);
        handler = g_fatalHandler;
    }
    if (handler)
    {
        handler(record);
    }
    DefaultFatalHandler(record);
}

} // namespace Detail
} // namespace Hylux
