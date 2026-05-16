/// @file
/// @brief StdFileSystem implementation.

#if defined(HYLUX_DESKTOP)

#include "Core/IO/Platform/Desktop/StdFileSystem.h"

#include "Core/IO/Platform/Desktop/StdFile.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace Hylux
{
namespace
{

std::filesystem::path ToPath(std::string_view path)
{
    return std::filesystem::path(std::string(path));
}

} // namespace

std::unique_ptr<IFile> StdFileSystem::Open(std::string_view path, FileOpenMode mode)
{
    auto file = std::make_unique<StdFile>(ToPath(path), mode);
    if (!file->IsValid()) return nullptr;
    return file;
}

bool StdFileSystem::Exists(std::string_view path) const
{
    std::error_code ec;
    return std::filesystem::exists(ToPath(path), ec);
}

FileStat StdFileSystem::Stat(std::string_view path) const
{
    FileStat out;
    std::error_code ec;
    const auto p = ToPath(path);
    const auto status = std::filesystem::status(p, ec);
    if (ec || !std::filesystem::exists(status)) return out;

    out.exists        = true;
    out.isDirectory   = std::filesystem::is_directory(status);
    out.isRegularFile = std::filesystem::is_regular_file(status);
    if (out.isRegularFile)
    {
        const auto sz = std::filesystem::file_size(p, ec);
        if (!ec) out.size = static_cast<FileSize>(sz);
    }
    return out;
}

bool StdFileSystem::CreateDirectories(std::string_view path)
{
    const auto p = ToPath(path);
    std::error_code ec;
    if (std::filesystem::exists(p, ec) && std::filesystem::is_directory(p, ec)) return true;
    std::filesystem::create_directories(p, ec);
    return !ec;
}

bool StdFileSystem::Remove(std::string_view path)
{
    std::error_code ec;
    return std::filesystem::remove(ToPath(path), ec) && !ec;
}

bool StdFileSystem::Rename(std::string_view from, std::string_view to)
{
    std::error_code ec;
    std::filesystem::rename(ToPath(from), ToPath(to), ec);
    return !ec;
}

bool StdFileSystem::IsAbsolute(std::string_view path) const
{
    return ToPath(path).is_absolute();
}

std::string StdFileSystem::JoinPath(std::string_view a, std::string_view b) const
{
    return (ToPath(a) / std::string(b)).string();
}

std::string StdFileSystem::CurrentDirectory() const
{
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) return {};
    return cwd.string();
}

} // namespace Hylux

#endif // HYLUX_DESKTOP
