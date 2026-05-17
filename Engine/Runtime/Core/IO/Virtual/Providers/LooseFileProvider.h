/// @file
/// @brief IFileProvider that maps a virtual mount onto a physical directory on disk.

#pragma once

#include "Core/IO/Virtual/IFileProvider.h"

#include <filesystem>
#include <memory>
#include <string>

namespace Hylux
{

class IFileSystem;

/// @brief Forwards VFS calls to the underlying physical filesystem (PhysicalFileSystem::Create()).
///        The root directory is set at construction; all sub-paths are joined onto it. Write-mode
///        opens auto-create missing parent directories so callers don't need to bracket every
///        write with CreateDirectories. Used in the editor for /Engine/ and /Game/ today; a
///        packaged build will swap these for PakFileProvider while keeping the same VFS API.
class LooseFileProvider final : public IFileProvider
{
public:
    /// @brief Constructs the provider rooted at @p rootDir. The directory is NOT required to exist
    ///        at construction time — Open(Write) will create it on demand. Reads against a missing
    ///        directory simply return nullptr.
    explicit LooseFileProvider(std::filesystem::path rootDir);
    ~LooseFileProvider() override;

    std::unique_ptr<IFile> Open(std::string_view subPath, FileOpenMode mode) override;
    bool                   Exists(std::string_view subPath) const override;
    FileStat               Stat(std::string_view subPath) const override;
    bool                   CreateDirectories(std::string_view subPath) override;
    bool                   Remove(std::string_view subPath) override;
    void                   EnumerateFiles(std::string_view subRoot,
                                          bool             recursive,
                                          std::function<void(std::string_view, const FileStat&)> visitor) const override;

    [[nodiscard]] bool        SupportsWrite() const noexcept override { return true; }
    [[nodiscard]] const char* DebugName() const noexcept override { return debugName_.c_str(); }

    [[nodiscard]] const std::filesystem::path& RootDir() const noexcept { return rootDir_; }

private:
    [[nodiscard]] std::string Join(std::string_view subPath) const;

    std::filesystem::path        rootDir_;
    std::unique_ptr<IFileSystem> physical_;
    std::string                  debugName_;
};

} // namespace Hylux
