/// @file
/// @brief CookedOrSourceProvider implementation.

#include "Core/IO/Virtual/Providers/CookedOrSourceProvider.h"

#include "Core/IO/IFile.h"

#include <string>
#include <unordered_set>

namespace Hylux
{
namespace
{

bool IsReadMode(FileOpenMode mode) noexcept
{
    return mode == FileOpenMode::Read;
}

} // namespace

CookedOrSourceProvider::CookedOrSourceProvider(std::shared_ptr<IFileProvider> cooked,
                                               std::shared_ptr<IFileProvider> source)
    : cooked_(std::move(cooked)), source_(std::move(source))
{
    debugName_ = "CookedOrSource(cooked=";
    debugName_ += cooked_ ? cooked_->DebugName() : "<null>";
    debugName_ += ", source=";
    debugName_ += source_ ? source_->DebugName() : "<null>";
    debugName_ += ')';
}

CookedOrSourceProvider::~CookedOrSourceProvider() = default;

std::unique_ptr<IFile> CookedOrSourceProvider::Open(std::string_view subPath, FileOpenMode mode)
{
    if (cooked_)
    {
        if (auto file = cooked_->Open(subPath, mode))
        {
            return file;
        }
    }
    if (!IsReadMode(mode))
    {
        return nullptr;
    }
    if (source_)
    {
        return source_->Open(subPath, mode);
    }
    return nullptr;
}

bool CookedOrSourceProvider::Exists(std::string_view subPath) const
{
    if (cooked_ && cooked_->Exists(subPath))
    {
        return true;
    }
    return source_ && source_->Exists(subPath);
}

FileStat CookedOrSourceProvider::Stat(std::string_view subPath) const
{
    if (cooked_)
    {
        FileStat s = cooked_->Stat(subPath);
        if (s.exists)
        {
            return s;
        }
    }
    if (source_)
    {
        return source_->Stat(subPath);
    }
    return {};
}

bool CookedOrSourceProvider::CreateDirectories(std::string_view subPath)
{
    return cooked_ && cooked_->CreateDirectories(subPath);
}

bool CookedOrSourceProvider::Remove(std::string_view subPath)
{
    return cooked_ && cooked_->Remove(subPath);
}

bool CookedOrSourceProvider::SupportsWrite() const noexcept
{
    return cooked_ && cooked_->SupportsWrite();
}

void CookedOrSourceProvider::EnumerateFiles(std::string_view subRoot, bool recursive,
                                            std::function<void(std::string_view, const FileStat&)> visitor) const
{
    if (!visitor)
    {
        return;
    }

    std::unordered_set<std::string> seenFromCooked;

    if (cooked_)
    {
        cooked_->EnumerateFiles(subRoot, recursive,
                                [&](std::string_view sub, const FileStat& stat) {
                                    seenFromCooked.emplace(sub);
                                    visitor(sub, stat);
                                });
    }

    if (!source_)
    {
        return;
    }

    source_->EnumerateFiles(subRoot, recursive,
                            [&](std::string_view sub, const FileStat& stat) {
                                if (seenFromCooked.find(std::string{sub}) != seenFromCooked.end())
                                {
                                    return;
                                }
                                visitor(sub, stat);
                            });
}

} // namespace Hylux
