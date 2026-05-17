/// @file
/// @brief Filesystem-backed IBlobStore. Logical keys are resolved as paths relative to
///        a root directory; OpenMapped returns a memory-mapped view of the underlying file.

#pragma once

#include "Core/IO/Blob/IBlobStore.h"

#include <filesystem>
#include <string>

namespace Hylux
{

/// @brief IBlobStore backed by a single root directory. Win32 backend uses
///        MapViewOfFile; POSIX uses mmap. Both fall back to a heap read on map failure.
///        Construction does not touch the filesystem; missing roots simply report
///        Exists()==false at lookup time.
class FilesystemBlobStore final : public IBlobStore
{
public:
    explicit FilesystemBlobStore(std::filesystem::path root);
    ~FilesystemBlobStore() override = default;

    [[nodiscard]] std::unique_ptr<IMappedBlob> OpenMapped(std::string_view logicalKey) override;
    [[nodiscard]] bool                         Exists(std::string_view logicalKey) const override;
    [[nodiscard]] std::optional<std::filesystem::file_time_type>
        LastModified(std::string_view logicalKey) const override;
    [[nodiscard]] std::string_view DebugName() const noexcept override { return debugName_; }

    /// @brief Returns the configured root directory.
    [[nodiscard]] const std::filesystem::path& Root() const noexcept { return root_; }

private:
    [[nodiscard]] std::filesystem::path Resolve(std::string_view logicalKey) const;

    std::filesystem::path root_;
    std::string           debugName_;
};

} // namespace Hylux
