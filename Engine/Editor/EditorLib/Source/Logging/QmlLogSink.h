/// @file
/// @brief Editor-side ILogSink that captures recent records into a thread-safe ring
///        buffer. LogViewModel polls this buffer on the Qt main thread and mirrors
///        new entries into a QAbstractListModel for QML consumption.

#pragma once

#include "Core/Logging/ILogSink.h"
#include "Core/Logging/LogLevel.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace Hylux::Editor
{

/// @brief Owned copy of a log entry (the source LogRecord borrows transient buffers).
struct LogEntrySnapshot
{
    std::uint64_t serial = 0;
    std::int64_t  timestampNs = 0;
    LogLevel      level{LogLevel::Info};
    std::string   category;
    std::string   message;
};

/// @brief Lock-protected ring buffer. Pushes happen from any logging thread; reads
///        happen from the Qt main thread (LogViewModel polls).
class QmlLogSink final : public ILogSink
{
public:
    explicit QmlLogSink(std::size_t capacity = 2048);
    ~QmlLogSink() override = default;

    QmlLogSink(const QmlLogSink&)            = delete;
    QmlLogSink& operator=(const QmlLogSink&) = delete;

    void Submit(const LogRecord& record) override;
    void Flush() override {}

    /// @brief Returns every entry with serial > sinceSerial in chronological order along
    ///        with the largest serial seen. Safe to call from any thread.
    [[nodiscard]] std::vector<LogEntrySnapshot> Snapshot(std::uint64_t sinceSerial, std::uint64_t& outLastSerial) const;

private:
    mutable std::mutex            mutex_;
    std::vector<LogEntrySnapshot> ring_;
    std::size_t                   capacity_;
    std::size_t                   nextIndex_  = 0;
    std::uint64_t                 nextSerial_ = 1;
    bool                          wrapped_    = false;
};

} // namespace Hylux::Editor
