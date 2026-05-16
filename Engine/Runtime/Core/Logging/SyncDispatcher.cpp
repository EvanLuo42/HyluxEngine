/// @file
/// @brief SyncDispatcher implementation.

#include "Core/Logging/SyncDispatcher.h"

#include <utility>

namespace Hylux
{

void SyncDispatcher::AddSink(std::unique_ptr<ILogSink> sink)
{
    if (sink)
    {
        sinks_.push_back(std::move(sink));
    }
}

void SyncDispatcher::Dispatch(const LogRecord& record)
{
    for (auto& sink : sinks_)
    {
        sink->Submit(record);
    }
}

void SyncDispatcher::Flush()
{
    for (auto& sink : sinks_)
    {
        sink->Flush();
    }
}

} // namespace Hylux
