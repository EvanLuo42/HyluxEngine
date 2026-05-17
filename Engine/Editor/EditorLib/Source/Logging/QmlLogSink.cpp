/// @file
/// @brief QmlLogSink implementation: ring-buffer of recent log entries.

#include "Logging/QmlLogSink.h"

#include <chrono>

namespace Hylux::Editor
{

QmlLogSink::QmlLogSink(std::size_t capacity) : capacity_(capacity == 0 ? 1 : capacity)
{
    ring_.resize(capacity_);
}

void QmlLogSink::Submit(const LogRecord& record)
{
    LogEntrySnapshot snap;
    snap.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           record.timestamp.time_since_epoch())
                           .count();
    snap.level    = record.level;
    snap.category = record.categoryName != nullptr ? std::string(record.categoryName) : std::string();
    snap.message  = std::string(record.message);

    std::lock_guard lock(mutex_);
    snap.serial          = nextSerial_++;
    ring_[nextIndex_]    = std::move(snap);
    nextIndex_           = (nextIndex_ + 1) % capacity_;
    if (nextIndex_ == 0)
    {
        wrapped_ = true;
    }
}

std::vector<LogEntrySnapshot> QmlLogSink::Snapshot(std::uint64_t sinceSerial, std::uint64_t& outLastSerial) const
{
    std::vector<LogEntrySnapshot> out;
    std::lock_guard               lock(mutex_);
    outLastSerial               = sinceSerial;
    const std::size_t entryCount = wrapped_ ? capacity_ : nextIndex_;
    out.reserve(entryCount);
    const std::size_t start = wrapped_ ? nextIndex_ : 0;
    for (std::size_t i = 0; i < entryCount; ++i)
    {
        const std::size_t idx = (start + i) % capacity_;
        const auto& entry = ring_[idx];
        if (entry.serial > sinceSerial)
        {
            out.push_back(entry);
            if (entry.serial > outLastSerial)
            {
                outLastSerial = entry.serial;
            }
        }
    }
    return out;
}

} // namespace Hylux::Editor
