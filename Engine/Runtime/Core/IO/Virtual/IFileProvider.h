/// @file
/// @brief Abstract source of files for a single VFS mount.

#pragma once

#include "Core/IO/FileMode.h"

#include <functional>
#include <memory>
#include <string_view>

namespace Hylux
{

class IFile;

/// @brief One mount source for the Virtual File System. Concrete implementations include
///        LooseFileProvider (forwards to a physical directory) and, in a future iteration,
///        PakFileProvider (reads from an .hpak archive). The VFS calls Open/Exists/Stat with
///        @p subPath — the portion of the virtual path *after* the mount prefix, with no
///        leading '/'. Errors are reported via nullptr/false; methods do not throw.
///        Implementations are not required to be thread-safe.
class IFileProvider
{
public:
    virtual ~IFileProvider() = default;

    IFileProvider(const IFileProvider&)            = delete;
    IFileProvider& operator=(const IFileProvider&) = delete;
    IFileProvider(IFileProvider&&)                 = delete;
    IFileProvider& operator=(IFileProvider&&)      = delete;

    /// @brief Opens the file at @p subPath relative to this provider's root. Returns nullptr if
    ///        the file does not exist (Read) or cannot be created/opened (Write/Append/ReadWrite).
    virtual std::unique_ptr<IFile> Open(std::string_view subPath, FileOpenMode mode) = 0;

    /// @brief Returns true if @p subPath names an existing file or directory under this provider.
    virtual bool Exists(std::string_view subPath) const = 0;

    /// @brief Returns metadata for @p subPath. On failure, FileStat::exists is false.
    virtual FileStat Stat(std::string_view subPath) const = 0;

    /// @brief Recursively creates the directory at @p subPath. Read-only providers return false.
    virtual bool CreateDirectories(std::string_view subPath) = 0;

    /// @brief Removes the file or empty directory at @p subPath. Read-only providers return false.
    virtual bool Remove(std::string_view subPath) = 0;

    /// @brief Walks every regular file under @p subRoot (forward-slash relative path, "" for
    ///        the provider's root). When @p recursive is true, descends into subdirectories.
    ///        The visitor is invoked once per file with a path relative to the provider root
    ///        (no leading slash) and a populated FileStat. Order is unspecified.
    virtual void EnumerateFiles(std::string_view subRoot,
                                bool             recursive,
                                std::function<void(std::string_view subPath, const FileStat& stat)> visitor) const = 0;

    /// @brief Returns true if this provider accepts write/append/read-write opens.
    [[nodiscard]] virtual bool SupportsWrite() const noexcept = 0;

    /// @brief Human-readable name used for logging and diagnostics. Lifetime is tied to the provider.
    [[nodiscard]] virtual const char* DebugName() const noexcept = 0;

protected:
    IFileProvider() = default;
};

} // namespace Hylux
