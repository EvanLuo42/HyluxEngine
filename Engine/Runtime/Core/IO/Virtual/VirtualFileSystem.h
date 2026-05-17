/// @file
/// @brief Engine subsystem implementing IVirtualFileSystem over a mount table of IFileProviders.

#pragma once

#include "Core/IO/Virtual/IVirtualFileSystem.h"
#include "Engine/ISubsystem.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Hylux
{

class IFileProvider;

/// @brief Concrete VFS subsystem. Implements both IVirtualFileSystem (mount management) and the
///        inherited IFileSystem read/write API. Construct via Engine::RegisterSubsystem; mount
///        providers any time after construction, before consumers expect to read.
///
///        Resolution: mount table is kept sorted by (prefix-length desc, priority desc,
///        mount-order desc). For a normalized path, every mount whose prefix matches is consulted
///        in sorted order. Reads fall through on miss; writes only attempt the highest-priority
///        writable provider that owns the path (no fall-through).
///
///        Main-thread only. No internal locking. Open IFile handles outlive Unmount because they
///        capture a shared_ptr to their provider.
class VirtualFileSystem final : public ISubsystem, public IVirtualFileSystem
{
public:
    VirtualFileSystem();
    ~VirtualFileSystem() override;

    void Initialize(Engine& engine) override;
    void Shutdown() override;

    MountId                Mount(std::string_view logicalPrefix,
                                 std::shared_ptr<IFileProvider> provider,
                                 std::int32_t priority) override;
    bool                   Unmount(MountId id) override;
    std::vector<MountInfo> EnumerateMounts() const override;
    void                   EnumerateFiles(std::string_view mountPrefix,
                                          bool             recursive,
                                          std::function<void(std::string_view, const FileStat&)> visitor) const override;

    std::unique_ptr<IFile> Open(std::string_view path, FileOpenMode mode) override;
    bool                   Exists(std::string_view path) const override;
    FileStat               Stat(std::string_view path) const override;
    bool                   CreateDirectories(std::string_view path) override;
    bool                   Remove(std::string_view path) override;
    bool                   Rename(std::string_view from, std::string_view to) override;

    bool        IsAbsolute(std::string_view path) const override;
    std::string JoinPath(std::string_view a, std::string_view b) const override;
    std::string CurrentDirectory() const override;

private:
    struct MountEntry
    {
        MountId                        id{MountId::Invalid};
        std::string                    prefix;
        std::int32_t                   priority{0};
        std::uint64_t                  mountOrder{0};
        std::shared_ptr<IFileProvider> provider;
    };

    void                       Resort();
    std::vector<const MountEntry*> CollectMatching(std::string_view normalizedPath,
                                                   std::string_view& outSub) const;
    const MountEntry*          FindWriter(std::string_view normalizedPath,
                                          std::string_view& outSub) const;

    std::vector<MountEntry> mounts_;
    std::atomic<std::uint64_t> nextMountId_{1};
    std::uint64_t              nextMountOrder_{0};
};

} // namespace Hylux
