/// @file
/// @brief Abstract file-handle interface returned by IFileSystem::Open.

#pragma once

#include "Core/IO/FileMode.h"

namespace Hylux
{

/// @brief A platform-agnostic handle to an open file. Instances are non-copyable, non-movable;
///        ownership is transferred via std::unique_ptr returned from IFileSystem::Open. The handle
///        is closed when the unique_ptr is destroyed. Implementations are NOT required to be
///        thread-safe — callers must serialize access externally.
class IFile
{
public:
    virtual ~IFile() = default;

    IFile(const IFile&)            = delete;
    IFile& operator=(const IFile&) = delete;
    IFile(IFile&&)                 = delete;
    IFile& operator=(IFile&&)      = delete;

    /// @brief Reads up to @p bytes into @p buffer. Returns the number of bytes actually read
    ///        (0 on EOF or error). Caller must allocate at least @p bytes.
    virtual FileSize Read(void* buffer, FileSize bytes) = 0;

    /// @brief Writes @p bytes from @p buffer. Returns the number of bytes actually written
    ///        (less than @p bytes indicates an I/O error).
    virtual FileSize Write(const void* buffer, FileSize bytes) = 0;

    /// @brief Repositions the cursor. Returns false on error.
    virtual bool Seek(FileOffset offset, SeekOrigin origin) = 0;

    /// @brief Returns the current cursor position in bytes from the start of the file,
    ///        or -1 on error.
    virtual FileOffset Tell() const = 0;

    /// @brief Flushes any buffered writes to the underlying device.
    virtual void Flush() = 0;

    /// @brief Returns the file size in bytes, or 0 on error.
    virtual FileSize Size() const = 0;

    /// @brief Returns true if the handle is open and usable.
    virtual bool IsValid() const noexcept = 0;

protected:
    IFile() = default;
};

} // namespace Hylux
