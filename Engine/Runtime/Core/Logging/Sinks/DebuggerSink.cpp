/// @file
/// @brief DebuggerSink implementation. Active only on Windows; empty TU elsewhere.

#include "Core/Logging/Sinks/DebuggerSink.h"

#if defined(HYLUX_WINDOWS)

#include <format>
#include <iterator>
#include <string>
#include <vector>

#include <windows.h>

namespace Hylux
{
namespace
{

std::wstring Utf8ToWide(std::string_view text)
{
    if (text.empty())
    {
        return {};
    }
    const int sizeNeeded = MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(), static_cast<int>(text.size()),
        nullptr, 0);
    if (sizeNeeded <= 0)
    {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(sizeNeeded), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(), static_cast<int>(text.size()),
        wide.data(), sizeNeeded);
    return wide;
}

} // namespace

void DebuggerSink::Submit(const LogRecord& record)
{
    if (!IsDebuggerPresent())
    {
        return;
    }
    std::string line;
    line.reserve(96 + record.message.size());
    std::format_to(std::back_inserter(line),
        "[{}] [{}] {}\n",
        LevelToString(record.level),
        record.categoryName,
        record.message);
    OutputDebugStringW(Utf8ToWide(line).c_str());
}

void DebuggerSink::Flush()
{
    // OutputDebugString is synchronous; nothing to flush.
}

} // namespace Hylux

#else

namespace Hylux
{

void DebuggerSink::Submit(const LogRecord&) {}
void DebuggerSink::Flush() {}

} // namespace Hylux

#endif
