/// @file
/// @brief Asynchronous dispatcher: producers enqueue, a dedicated worker thread writes to sinks.

#pragma once

#include "Core/Logging/ILogSink.h"
#include "Core/Logging/LogDispatcher.h"
#include "Core/Logging/LogRecord.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Hylux
{

/// @brief Multi-producer single-consumer log dispatcher. Dispatch returns after copying the
///        message into an internal queue; a worker thread drains the queue and calls Submit
///        on each sink. Flush blocks until every record submitted prior to the call has been
///        written, then calls Flush on each sink.
class AsyncDispatcher final : public LogDispatcher
{
public:
    AsyncDispatcher();
    ~AsyncDispatcher() override;
    AsyncDispatcher(const AsyncDispatcher&) = delete;
    AsyncDispatcher& operator=(const AsyncDispatcher&) = delete;
    AsyncDispatcher(AsyncDispatcher&&) = delete;
    AsyncDispatcher& operator=(AsyncDispatcher&&) = delete;

    /// @brief Adds a sink. Must be called before the dispatcher is exposed to other threads.
    void AddSink(std::unique_ptr<ILogSink> sink);

    /// @brief Starts the worker thread. Call once after all sinks are added.
    void Start();

    void Dispatch(const LogRecord& record) override;
    void Flush() override;

private:
    struct Entry
    {
        std::chrono::system_clock::time_point timestamp;
        std::thread::id                       threadId;
        LogLevel                              level;
        const char*                           categoryName;
        LogSite                               site;
        std::string                           message;
    };

    void WorkerLoop();

    std::vector<std::unique_ptr<ILogSink>> sinks_;

    std::mutex                             mutex_;
    std::condition_variable                producerCv_;
    std::condition_variable                drainCv_;
    std::deque<Entry>                      queue_;
    std::uint64_t                          produced_{0};
    std::uint64_t                          processed_{0};
    std::atomic<bool>                      stop_{false};
    std::thread                            worker_;
};

} // namespace Hylux
