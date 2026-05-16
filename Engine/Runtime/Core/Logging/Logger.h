/// @file
/// @brief Public Logger API: HYLUX_LOG macro family, fatal handler hook, dispatcher swap.

#pragma once

#include "Core/Logging/LogCategory.h"
#include "Core/Logging/LogDispatcher.h"
#include "Core/Logging/LogRecord.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#if defined(_MSC_VER)
    #define HYLUX_FUNCSIG __FUNCSIG__
#else
    #define HYLUX_FUNCSIG __PRETTY_FUNCTION__
#endif

#ifndef HYLUX_LOG_LEVEL_MIN
    #define HYLUX_LOG_LEVEL_MIN 0
#endif

namespace Hylux::Logging
{

using FatalHandler = std::function<void(const LogRecord&)>;

/// @brief Installs a handler invoked after a Fatal log is dispatched. Default flushes and aborts.
void SetFatalHandler(FatalHandler handler);

/// @brief Swaps the active dispatcher. Returns the previous one (never null).
///        Ownership is not transferred; the caller must keep the new dispatcher alive
///        until a subsequent swap restores something else.
LogDispatcher* SetActiveDispatcher(LogDispatcher* dispatcher) noexcept;

/// @brief Returns the active dispatcher pointer. Always non-null.
[[nodiscard]] LogDispatcher* GetActiveDispatcher() noexcept;

/// @brief Restores the built-in bootstrap dispatcher (stderr under mutex). Used during teardown.
LogDispatcher* RestoreBootstrapDispatcher() noexcept;

} // namespace Hylux::Logging

namespace Hylux::Detail
{

/// @brief Returns a thread-local string buffer reused by every log call on this thread.
[[nodiscard]] std::string& LogFormatBuffer() noexcept;

/// @brief Invokes the active fatal handler after dispatching a Fatal record.
[[noreturn]] void InvokeFatalHandler(const LogRecord& record);

/// @brief Common dispatch path: formats into the thread-local buffer, builds a LogRecord,
///        sends it through the active dispatcher, and on Fatal escalates to the handler.
template<typename... Args>
void DispatchFormat(const LogCategoryBase& category, LogLevel level, LogSite site,
                    std::format_string<Args...> fmt, Args&&... args)
{
    std::string& buffer = LogFormatBuffer();
    buffer.clear();
    std::vformat_to(std::back_inserter(buffer), fmt.get(),
                    std::make_format_args(args...));

    const LogRecord record{
        std::chrono::system_clock::now(),
        std::this_thread::get_id(),
        level,
        category.name,
        site,
        std::string_view{buffer}
    };

    ::Hylux::Logging::GetActiveDispatcher()->Dispatch(record);

    if (level == LogLevel::Fatal)
    {
        InvokeFatalHandler(record);
    }
}

} // namespace Hylux::Detail

/// @brief Emits a log record. Trace/Debug calls below HYLUX_LOG_LEVEL_MIN or the category's
///        compile-time minimum compile out entirely (no string literal, no argument evaluation).
#define HYLUX_LOG(Category, Level, ...)                                                            \
    do {                                                                                           \
        if constexpr (static_cast<int>(::Hylux::LogLevel::Level) >= HYLUX_LOG_LEVEL_MIN &&         \
                      static_cast<int>(::Hylux::LogLevel::Level) >=                                \
                      static_cast<int>(                                                            \
                          std::remove_reference_t<decltype((Category))>::kCompileMin))             \
        {                                                                                          \
            if ((Category).runtimeMin.load(std::memory_order_relaxed) <=                           \
                ::Hylux::LogLevel::Level)                                                          \
            {                                                                                      \
                ::Hylux::Detail::DispatchFormat(                                                   \
                    (Category), ::Hylux::LogLevel::Level,                                          \
                    ::Hylux::LogSite{__FILE__, __LINE__, HYLUX_FUNCSIG},                           \
                    __VA_ARGS__);                                                                  \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/// @brief Like HYLUX_LOG but emits at most once per call site for the life of the process.
#define HYLUX_LOG_ONCE(Category, Level, ...)                                                       \
    do {                                                                                           \
        static ::std::atomic<bool> hyluxLogOnceFired_{false};                                      \
        if (!hyluxLogOnceFired_.exchange(true, ::std::memory_order_relaxed))                       \
        {                                                                                          \
            HYLUX_LOG(Category, Level, __VA_ARGS__);                                               \
        }                                                                                          \
    } while (0)

/// @brief Emits at most once every N invocations of this call site (1-based, fires on the Nth).
#define HYLUX_LOG_EVERY_N(Category, Level, N, ...)                                                 \
    do {                                                                                           \
        static ::std::atomic<::std::uint64_t> hyluxLogEveryCounter_{0};                            \
        const auto hyluxLogIdx_ =                                                                  \
            hyluxLogEveryCounter_.fetch_add(1, ::std::memory_order_relaxed);                       \
        if (hyluxLogIdx_ % static_cast<::std::uint64_t>(N) == 0)                                   \
        {                                                                                          \
            HYLUX_LOG(Category, Level, __VA_ARGS__);                                               \
        }                                                                                          \
    } while (0)

#define HYLUX_LOG_TRACE(Category, ...) HYLUX_LOG(Category, Trace, __VA_ARGS__)
#define HYLUX_LOG_DEBUG(Category, ...) HYLUX_LOG(Category, Debug, __VA_ARGS__)
#define HYLUX_LOG_INFO(Category, ...)  HYLUX_LOG(Category, Info,  __VA_ARGS__)
#define HYLUX_LOG_WARN(Category, ...)  HYLUX_LOG(Category, Warn,  __VA_ARGS__)
#define HYLUX_LOG_ERROR(Category, ...) HYLUX_LOG(Category, Error, __VA_ARGS__)
#define HYLUX_LOG_FATAL(Category, ...) HYLUX_LOG(Category, Fatal, __VA_ARGS__)
