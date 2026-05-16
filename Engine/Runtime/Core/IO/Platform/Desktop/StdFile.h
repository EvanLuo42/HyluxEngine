/// @file
/// @brief Desktop (Windows/Linux) IFile implementation backed by std::fstream.

#pragma once

#if defined(HYLUX_DESKTOP)

#include "Core/IO/IFile.h"

#include <filesystem>
#include <fstream>

namespace Hylux
{

/// @brief IFile implementation that wraps std::fstream. Used on desktop platforms (Windows, Linux).
///        Not thread-safe; callers must serialize access.
class StdFile final : public IFile
{
public:
    /// @brief Opens @p path with the requested @p mode. After construction, IsValid() reports
    ///        whether the underlying stream is open.
    StdFile(const std::filesystem::path& path, FileOpenMode mode);
    ~StdFile() override;

    FileSize   Read(void* buffer, FileSize bytes) override;
    FileSize   Write(const void* buffer, FileSize bytes) override;
    bool       Seek(FileOffset offset, SeekOrigin origin) override;
    FileOffset Tell() const override;
    void       Flush() override;
    FileSize   Size() const override;
    bool       IsValid() const noexcept override;

private:
    mutable std::fstream stream_;
    FileOpenMode         mode_;
};

} // namespace Hylux

#endif // HYLUX_DESKTOP
