/// @file
/// @brief ConsoleSink implementation with per-platform color setup.

#include "Core/Logging/Sinks/ConsoleSink.h"

#include "Core/Utils/Assert.h"

#include <chrono>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#if defined(HYLUX_WINDOWS)
    #include <windows.h>
#elif defined(HYLUX_LINUX)
    #include <unistd.h>
#endif

namespace Hylux
{
namespace
{

constexpr std::string_view kAnsiReset = "\x1b[0m";

constexpr std::string_view AnsiColorFor(LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::Trace: return "\x1b[90m";
        case LogLevel::Debug: return "\x1b[36m";
        case LogLevel::Info:  return "\x1b[0m";
        case LogLevel::Warn:  return "\x1b[33m";
        case LogLevel::Error: return "\x1b[31m";
        case LogLevel::Fatal: return "\x1b[1;31m";
        case LogLevel::Off:   return "\x1b[0m";
    }
    return "\x1b[0m";
}

void FormatLine(std::string& out, const LogRecord& record)
{
    using namespace std::chrono;
    const auto local = current_zone()->to_local(record.timestamp);
    const auto day   = floor<days>(local);
    const auto tod   = hh_mm_ss{floor<milliseconds>(local - day)};
    std::format_to(std::back_inserter(out),
        "{:02}:{:02}:{:02}.{:03} [{}] [{}] {}\n",
        tod.hours().count(), tod.minutes().count(), tod.seconds().count(),
        tod.subseconds().count(),
        LevelToString(record.level),
        record.categoryName,
        record.message);
}

} // namespace

ConsoleSink::ConsoleSink()
{
#if defined(HYLUX_WINDOWS)
    // GUI-subsystem executables have no stdout/stderr handles by default.
    // Attach to the parent console if launched from cmd / IDE terminal; in Debug builds
    // fall back to allocating a fresh console window so logs are visible during dev.
    if (GetConsoleWindow() == nullptr)
    {
        bool attached = (AttachConsole(ATTACH_PARENT_PROCESS) != FALSE);
        if constexpr (Diagnostics::kBuildIsDebug)
        {
            if (!attached)
            {
                attached = (AllocConsole() != FALSE);
            }
        }
        if (attached)
        {
            FILE* dummy = nullptr;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
        }
    }

    SetConsoleOutputCP(CP_UTF8);
    HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode      = 0;
    if (outHandle != INVALID_HANDLE_VALUE && GetConsoleMode(outHandle, &mode))
    {
        SetConsoleMode(outHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        colorsEnabled_ = true;
    }
    HANDLE errHandle = GetStdHandle(STD_ERROR_HANDLE);
    if (errHandle != INVALID_HANDLE_VALUE && GetConsoleMode(errHandle, &mode))
    {
        SetConsoleMode(errHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#elif defined(HYLUX_LINUX)
    colorsEnabled_ = (isatty(fileno(stdout)) != 0);
#else
    colorsEnabled_ = false;
#endif
}

void ConsoleSink::Submit(const LogRecord& record)
{
    std::string line;
    line.reserve(128 + record.message.size());
    FormatLine(line, record);

    std::FILE* target = (record.level >= LogLevel::Warn) ? stderr : stdout;

    std::lock_guard<std::mutex> guard(mutex_);
    if (colorsEnabled_)
    {
        const std::string_view color = AnsiColorFor(record.level);
        std::fwrite(color.data(), 1, color.size(), target);
        std::fwrite(line.data(),  1, line.size(),  target);
        std::fwrite(kAnsiReset.data(), 1, kAnsiReset.size(), target);
    }
    else
    {
        std::fwrite(line.data(), 1, line.size(), target);
    }
}

void ConsoleSink::Flush()
{
    std::lock_guard<std::mutex> guard(mutex_);
    std::fflush(stdout);
    std::fflush(stderr);
}

} // namespace Hylux
