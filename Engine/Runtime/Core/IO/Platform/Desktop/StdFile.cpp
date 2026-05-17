/// @file
/// @brief StdFile implementation.

#if defined(HYLUX_DESKTOP)

#include "Core/IO/Platform/Desktop/StdFile.h"

#include <ios>

namespace Hylux
{
namespace
{

std::ios::openmode ToStdOpenMode(FileOpenMode mode)
{
    switch (mode)
    {
    case FileOpenMode::Read:
        return std::ios::in | std::ios::binary;
    case FileOpenMode::Write:
        return std::ios::out | std::ios::binary | std::ios::trunc;
    case FileOpenMode::Append:
        return std::ios::out | std::ios::binary | std::ios::app;
    case FileOpenMode::ReadWrite:
        return std::ios::in | std::ios::out | std::ios::binary;
    }
    return std::ios::binary;
}

std::ios::seekdir ToStdSeekDir(SeekOrigin origin)
{
    switch (origin)
    {
    case SeekOrigin::Begin:   return std::ios::beg;
    case SeekOrigin::Current: return std::ios::cur;
    case SeekOrigin::End:     return std::ios::end;
    }
    return std::ios::beg;
}

} // namespace

StdFile::StdFile(const std::filesystem::path& path, FileOpenMode mode)
    : mode_(mode)
{
    const std::ios::openmode flags = ToStdOpenMode(mode);

    if (mode == FileOpenMode::ReadWrite && !std::filesystem::exists(path))
    {
        // Touch the file so that ReadWrite (in|out) can open it without truncation.
        std::ofstream create(path, std::ios::out | std::ios::binary);
    }

    stream_.open(path, flags);
}

StdFile::~StdFile()
{
    if (stream_.is_open())
    {
        stream_.flush();
        stream_.close();
    }
}

FileSize StdFile::Read(void* buffer, FileSize bytes)
{
    if (!stream_.is_open() || bytes == 0) return 0;
    stream_.read(static_cast<char*>(buffer), static_cast<std::streamsize>(bytes));
    const auto got = stream_.gcount();
    if (stream_.eof()) stream_.clear(stream_.rdstate() & ~std::ios::failbit);
    return static_cast<FileSize>(got);
}

FileSize StdFile::Write(const void* buffer, FileSize bytes)
{
    if (!stream_.is_open() || bytes == 0) return 0;
    stream_.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(bytes));
    return stream_.good() ? bytes : 0;
}

bool StdFile::Seek(FileOffset offset, SeekOrigin origin)
{
    if (!stream_.is_open()) return false;
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), ToStdSeekDir(origin));
    // The get and put pointers share the underlying file cursor; calling seekp with
    // the same relative offset would double-advance for SeekOrigin::Current. Sync
    // the put pointer to the absolute landing position instead.
    const auto pos = stream_.tellg();
    if (pos != std::fstream::pos_type(-1))
    {
        stream_.seekp(pos);
    }
    return stream_.good();
}

FileOffset StdFile::Tell() const
{
    if (!stream_.is_open()) return -1;
    const auto pos = stream_.tellg();
    if (pos == std::fstream::pos_type(-1)) return -1;
    return static_cast<FileOffset>(pos);
}

void StdFile::Flush()
{
    if (stream_.is_open()) stream_.flush();
}

FileSize StdFile::Size() const
{
    if (!stream_.is_open()) return 0;
    const auto saved = stream_.tellg();
    stream_.seekg(0, std::ios::end);
    const auto end = stream_.tellg();
    stream_.seekg(saved, std::ios::beg);
    if (end == std::fstream::pos_type(-1)) return 0;
    return static_cast<FileSize>(end);
}

bool StdFile::IsValid() const noexcept
{
    return stream_.is_open();
}

} // namespace Hylux

#endif // HYLUX_DESKTOP
