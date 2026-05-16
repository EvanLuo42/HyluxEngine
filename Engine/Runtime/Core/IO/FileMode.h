/// @file
/// @brief Open-mode, seek-origin, and size/offset typedefs shared by IFile / IFileSystem.

#pragma once

#include <cstdint>

namespace Hylux
{

/// @brief How an IFile should be opened.
enum class FileOpenMode : std::uint8_t
{
    Read,       ///< Read-only; the file must already exist.
    Write,      ///< Write-only; truncates existing content, creates if missing.
    Append,     ///< Write-only; positions at end, creates if missing.
    ReadWrite,  ///< Read+write; creates if missing, does not truncate.
};

/// @brief Reference point for IFile::Seek.
enum class SeekOrigin : std::uint8_t
{
    Begin,
    Current,
    End,
};

/// @brief Signed byte offset used for relative positioning. 64-bit on all platforms.
using FileOffset = std::int64_t;

/// @brief Unsigned byte count used for sizes and lengths. 64-bit on all platforms.
using FileSize = std::uint64_t;

/// @brief Lightweight stat result returned by IFileSystem::Stat.
struct FileStat
{
    FileSize size{0};
    bool     exists{false};
    bool     isDirectory{false};
    bool     isRegularFile{false};
};

} // namespace Hylux
