/// @file
/// @brief File sink: writes one log file per process with full ISO timestamps.

#pragma once

#include "Core/Logging/ILogSink.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace Hylux
{

class IFile;
class IFileSystem;

/// @brief Appends log lines to a file. The path is fixed at construction; one process produces one
///        log file, named Hylux-YYYYMMDD-HHMMSS.log under the configured directory. File I/O goes
///        through PhysicalFileSystem so the sink itself has no dependency on <fstream>/<filesystem>.
class FileSink final : public ILogSink
{
public:
    /// @brief Opens (and creates the directory for) a log file under @p directory.
    ///        If opening fails the sink silently drops all records — IO errors at logger init
    ///        should not block engine startup.
    explicit FileSink(std::string_view directory);
    ~FileSink() override;
    FileSink(const FileSink&) = delete;
    FileSink& operator=(const FileSink&) = delete;

    void Submit(const LogRecord& record) override;
    void Flush() override;

    /// @brief Returns the absolute path of the opened log file, or empty if open failed.
    [[nodiscard]] std::string_view FilePath() const noexcept { return filePath_; }

private:
    std::mutex                   mutex_;
    std::unique_ptr<IFileSystem> fileSystem_;
    std::unique_ptr<IFile>       file_;
    std::string                  filePath_;
};

} // namespace Hylux
