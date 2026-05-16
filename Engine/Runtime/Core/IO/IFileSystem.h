/// @file
/// @brief Abstract filesystem interface. Paths are UTF-8 std::string_view.

#pragma once

#include "Core/IO/FileMode.h"

#include <memory>
#include <string>
#include <string_view>

namespace Hylux
{

class IFile;

/// @brief Platform-agnostic filesystem façade. Concrete implementations live under
///        Core/IO/Platform/<PlatformName>/ and are produced by PhysicalFileSystem::Create.
///        All path arguments are UTF-8 encoded std::string_view. Methods do not throw — failures
///        are reported via nullptr / false / empty return values.
class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    IFileSystem(const IFileSystem&)            = delete;
    IFileSystem& operator=(const IFileSystem&) = delete;
    IFileSystem(IFileSystem&&)                 = delete;
    IFileSystem& operator=(IFileSystem&&)      = delete;

    /// @brief Opens @p path with the requested @p mode. Returns nullptr on failure.
    virtual std::unique_ptr<IFile> Open(std::string_view path, FileOpenMode mode) = 0;

    /// @brief Returns true if @p path names an existing file or directory.
    virtual bool Exists(std::string_view path) const = 0;

    /// @brief Returns metadata for @p path. On failure, FileStat::exists is false.
    virtual FileStat Stat(std::string_view path) const = 0;

    /// @brief Recursively creates @p path. Returns true if the directory exists after the call
    ///        (whether it was created now or already present).
    virtual bool CreateDirectories(std::string_view path) = 0;

    /// @brief Removes a file or empty directory. Returns false on failure.
    virtual bool Remove(std::string_view path) = 0;

    /// @brief Renames or moves @p from to @p to. Returns false on failure.
    virtual bool Rename(std::string_view from, std::string_view to) = 0;

    /// @brief Returns true if @p path is absolute according to platform rules.
    virtual bool IsAbsolute(std::string_view path) const = 0;

    /// @brief Joins two path fragments using the platform's preferred separator.
    virtual std::string JoinPath(std::string_view a, std::string_view b) const = 0;

    /// @brief Returns the process current working directory as an absolute path.
    virtual std::string CurrentDirectory() const = 0;

protected:
    IFileSystem() = default;
};

} // namespace Hylux
