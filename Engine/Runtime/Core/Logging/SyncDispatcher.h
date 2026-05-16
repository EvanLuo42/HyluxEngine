/// @file
/// @brief Synchronous dispatcher: calling thread walks every sink and writes inline.

#pragma once

#include "Core/Logging/ILogSink.h"
#include "Core/Logging/LogDispatcher.h"

#include <memory>
#include <vector>

namespace Hylux
{

/// @brief Dispatches records on the calling thread. Sinks are responsible for their own locking.
///        Sinks must be added before the dispatcher is published; the sink list is immutable
///        after the first concurrent Dispatch call.
class SyncDispatcher final : public LogDispatcher
{
public:
    SyncDispatcher() = default;
    ~SyncDispatcher() override = default;
    SyncDispatcher(const SyncDispatcher&) = delete;
    SyncDispatcher& operator=(const SyncDispatcher&) = delete;
    SyncDispatcher(SyncDispatcher&&) = delete;
    SyncDispatcher& operator=(SyncDispatcher&&) = delete;

    /// @brief Adds a sink. Must be called before the dispatcher is exposed to other threads.
    void AddSink(std::unique_ptr<ILogSink> sink);

    void Dispatch(const LogRecord& record) override;
    void Flush() override;

private:
    std::vector<std::unique_ptr<ILogSink>> sinks_;
};

} // namespace Hylux
