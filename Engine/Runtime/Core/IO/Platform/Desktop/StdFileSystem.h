/// @file
/// @brief Desktop (Windows/Linux) IFileSystem implementation backed by std::filesystem.

#pragma once

#if defined(HYLUX_DESKTOP)

#include "Core/IO/IFileSystem.h"

namespace Hylux
{

/// @brief IFileSystem implementation that delegates to std::filesystem and std::fstream.
///        Used on desktop platforms (Windows, Linux). UTF-8 in/out; relies on the MSVC /utf-8
///        flag (or the platform default on POSIX) to round-trip non-ASCII paths.
class StdFileSystem final : public IFileSystem
{
public:
    StdFileSystem()           = default;
    ~StdFileSystem() override = default;

    std::unique_ptr<IFile> Open(std::string_view path, FileOpenMode mode) override;

    bool     Exists(std::string_view path) const override;
    FileStat Stat(std::string_view path) const override;
    bool     CreateDirectories(std::string_view path) override;
    bool     Remove(std::string_view path) override;
    bool     Rename(std::string_view from, std::string_view to) override;

    bool        IsAbsolute(std::string_view path) const override;
    std::string JoinPath(std::string_view a, std::string_view b) const override;
    std::string CurrentDirectory() const override;
};

} // namespace Hylux

#endif // HYLUX_DESKTOP
