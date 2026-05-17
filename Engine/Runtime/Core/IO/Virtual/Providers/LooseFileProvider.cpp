/// @file
/// @brief LooseFileProvider implementation.

#include "Core/IO/Virtual/Providers/LooseFileProvider.h"

#include "Core/IO/IFile.h"
#include "Core/IO/IFileSystem.h"
#include "Core/IO/PhysicalFileSystem.h"

#include <system_error>
#include <utility>

namespace Hylux
{

LooseFileProvider::LooseFileProvider(std::filesystem::path rootDir)
    : rootDir_(std::move(rootDir))
    , physical_(PhysicalFileSystem::Create())
    , debugName_("Loose(" + rootDir_.string() + ")")
{
}

LooseFileProvider::~LooseFileProvider() = default;

std::string LooseFileProvider::Join(std::string_view subPath) const
{
    if (subPath.empty()) return rootDir_.string();
    return (rootDir_ / std::filesystem::path(std::string(subPath))).string();
}

std::unique_ptr<IFile> LooseFileProvider::Open(std::string_view subPath, FileOpenMode mode)
{
    if (!physical_) return nullptr;

    const std::string fullPath = Join(subPath);

    const bool wantsWrite = mode == FileOpenMode::Write || mode == FileOpenMode::Append ||
                            mode == FileOpenMode::ReadWrite;
    if (wantsWrite)
    {
        const auto parent = std::filesystem::path(fullPath).parent_path();
        if (!parent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
        }
    }

    return physical_->Open(fullPath, mode);
}

bool LooseFileProvider::Exists(std::string_view subPath) const
{
    return physical_ && physical_->Exists(Join(subPath));
}

FileStat LooseFileProvider::Stat(std::string_view subPath) const
{
    if (!physical_) return {};
    return physical_->Stat(Join(subPath));
}

bool LooseFileProvider::CreateDirectories(std::string_view subPath)
{
    return physical_ && physical_->CreateDirectories(Join(subPath));
}

bool LooseFileProvider::Remove(std::string_view subPath)
{
    return physical_ && physical_->Remove(Join(subPath));
}

void LooseFileProvider::EnumerateFiles(std::string_view subRoot, bool recursive,
                                       std::function<void(std::string_view, const FileStat&)> visitor) const
{
    if (!visitor)
    {
        return;
    }

    const std::filesystem::path scanRoot =
        subRoot.empty() ? rootDir_ : (rootDir_ / std::filesystem::path(std::string(subRoot)));

    std::error_code rootEc;
    if (!std::filesystem::exists(scanRoot, rootEc) || !std::filesystem::is_directory(scanRoot, rootEc))
    {
        return;
    }

    auto emit = [&](const std::filesystem::directory_entry& entry) {
        std::error_code ec;
        if (!entry.is_regular_file(ec))
        {
            return;
        }
        std::error_code relEc;
        const auto      relPath = std::filesystem::relative(entry.path(), rootDir_, relEc);
        if (relEc)
        {
            return;
        }
        std::string relStr = relPath.generic_string();

        FileStat stat{};
        stat.exists        = true;
        stat.isRegularFile = true;
        stat.isDirectory   = false;
        stat.size          = static_cast<FileSize>(entry.file_size(ec));

        visitor(relStr, stat);
    };

    std::error_code iterEc;
    if (recursive)
    {
        for (std::filesystem::recursive_directory_iterator it(scanRoot, iterEc), end; it != end;
             it.increment(iterEc))
        {
            if (iterEc)
            {
                return;
            }
            emit(*it);
        }
    }
    else
    {
        for (std::filesystem::directory_iterator it(scanRoot, iterEc), end; it != end;
             it.increment(iterEc))
        {
            if (iterEc)
            {
                return;
            }
            emit(*it);
        }
    }
}

} // namespace Hylux
