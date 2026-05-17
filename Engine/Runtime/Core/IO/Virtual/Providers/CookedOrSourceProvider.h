/// @file
/// @brief IFileProvider that composes two child providers — a "cooked" root and a
///        "source" root — under a single VFS mount. Reads try cooked first and fall
///        through to source on miss; writes go only to cooked. Lets the editor expose
///        one logical mount per content tree (e.g. /Game/) while keeping cooked binaries
///        and human-authored sources visible at their natural paths.

#pragma once

#include "Core/IO/Virtual/IFileProvider.h"

#include <memory>
#include <string>
#include <utility>

namespace Hylux
{

/// @brief Two-tier file provider. The cooked provider has priority for every operation;
///        the source provider is a read-only fallback used only when the cooked provider
///        reports the file as absent. Writes never fall through (matches the VFS rule
///        that writes target the single highest-priority writable provider).
///
///        Typical editor wiring:
///            vfs->Mount("/Game/", std::make_shared<CookedOrSourceProvider>(
///                std::make_shared<LooseFileProvider>(savedCookedRoot / "Game"),
///                std::make_shared<LooseFileProvider>(contentRoot)
///            ), 0);
class CookedOrSourceProvider final : public IFileProvider
{
public:
    /// @brief Constructs the provider. `cooked` must be non-null; `source` may be null
    ///        (shipped builds) in which case the provider behaves like cooked alone.
    CookedOrSourceProvider(std::shared_ptr<IFileProvider> cooked, std::shared_ptr<IFileProvider> source);
    ~CookedOrSourceProvider() override;

    std::unique_ptr<IFile> Open(std::string_view subPath, FileOpenMode mode) override;
    bool                   Exists(std::string_view subPath) const override;
    FileStat               Stat(std::string_view subPath) const override;
    bool                   CreateDirectories(std::string_view subPath) override;
    bool                   Remove(std::string_view subPath) override;
    void                   EnumerateFiles(std::string_view subRoot,
                                          bool             recursive,
                                          std::function<void(std::string_view, const FileStat&)> visitor) const override;

    [[nodiscard]] bool        SupportsWrite() const noexcept override;
    [[nodiscard]] const char* DebugName() const noexcept override { return debugName_.c_str(); }

    [[nodiscard]] IFileProvider* Cooked() const noexcept { return cooked_.get(); }
    [[nodiscard]] IFileProvider* Source() const noexcept { return source_.get(); }

private:
    std::shared_ptr<IFileProvider> cooked_;
    std::shared_ptr<IFileProvider> source_;
    std::string                    debugName_;
};

} // namespace Hylux
