/// @file
/// @brief AsyncDispatcher implementation.

#include "Core/Logging/AsyncDispatcher.h"

#include <utility>

namespace Hylux
{

AsyncDispatcher::AsyncDispatcher() = default;

AsyncDispatcher::~AsyncDispatcher()
{
    {
        std::lock_guard<std::mutex> guard(mutex_);
        stop_.store(true, std::memory_order_release);
    }
    producerCv_.notify_all();
    if (worker_.joinable())
    {
        worker_.join();
    }
    for (auto& sink : sinks_)
    {
        sink->Flush();
    }
}

void AsyncDispatcher::AddSink(std::unique_ptr<ILogSink> sink)
{
    if (sink)
    {
        sinks_.push_back(std::move(sink));
    }
}

void AsyncDispatcher::Start()
{
    worker_ = std::thread(&AsyncDispatcher::WorkerLoop, this);
}

void AsyncDispatcher::Dispatch(const LogRecord& record)
{
    {
        std::lock_guard<std::mutex> guard(mutex_);
        queue_.push_back(Entry{
            record.timestamp,
            record.threadId,
            record.level,
            record.categoryName,
            record.site,
            std::string{record.message}
        });
        ++produced_;
    }
    producerCv_.notify_one();
}

void AsyncDispatcher::Flush()
{
    std::uint64_t target;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        target = produced_;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        drainCv_.wait(lock, [this, target] {
            return processed_ >= target || stop_.load(std::memory_order_acquire);
        });
    }
    for (auto& sink : sinks_)
    {
        sink->Flush();
    }
}

void AsyncDispatcher::WorkerLoop()
{
    for (;;)
    {
        Entry entry;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            producerCv_.wait(lock, [this] {
                return !queue_.empty() || stop_.load(std::memory_order_acquire);
            });
            if (queue_.empty())
            {
                drainCv_.notify_all();
                return;
            }
            entry = std::move(queue_.front());
            queue_.pop_front();
        }

        const LogRecord record{
            entry.timestamp,
            entry.threadId,
            entry.level,
            entry.categoryName,
            entry.site,
            std::string_view{entry.message}
        };
        for (auto& sink : sinks_)
        {
            sink->Submit(record);
        }

        {
            std::lock_guard<std::mutex> guard(mutex_);
            ++processed_;
        }
        drainCv_.notify_all();
    }
}

} // namespace Hylux
