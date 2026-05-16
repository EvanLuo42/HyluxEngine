/// @file
/// @brief Virtual File System interface: IFileSystem + mount-point management.

#pragma once

#include "Core/IO/IFileSystem.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Hylux
{

class IFileProvider;

/// @brief Opaque identifier returned by Mount; pass back to Unmount.
enum class MountId : std::uint64_t
{
    Invalid = 0
};

/// @brief Snapshot of a single active mount, returned by EnumerateMounts.
struct MountInfo
{
    MountId       id{MountId::Invalid};
    std::string   prefix;
    std::int32_t  priority{0};
    std::uint64_t mountOrder{0};
    const char*   providerName{nullptr};
};

/// @brief A composable filesystem that routes paths like "/Engine/Shaders/Common.glsl" to
///        one of several mounted providers. IS-A IFileSystem so any consumer that holds an
///        IFileSystem* can read VFS-mounted content with the same calls used for raw disk.
///
///        Resolution rules (see VirtualFileSystem.cpp for the authoritative implementation):
///          - longest mount-prefix wins; among equal-length prefixes, higher priority wins;
///            among equal priorities, the most recently mounted provider wins.
///          - Read opens fall through to the next provider on miss.
///          - Write/Append/ReadWrite opens only attempt the highest-priority writable provider;
///            no fall-through. Silent fallthrough on writes is a debugging trap.
///
///        All methods must be called from the same thread (the engine main loop).
class IVirtualFileSystem : public IFileSystem
{
public:
    /// @brief Registers @p provider under @p logicalPrefix. The prefix must be "/<Name>/" with no
    ///        embedded '/'. Higher @p priority shadows lower at the same prefix. Returns
    ///        MountId::Invalid on failure (invalid prefix, null provider).
    virtual MountId Mount(std::string_view logicalPrefix,
                          std::shared_ptr<IFileProvider> provider,
                          std::int32_t priority) = 0;

    /// @brief Removes the mount identified by @p id. Returns false if @p id is unknown. Open files
    ///        spawned from the provider continue to function until they are closed.
    virtual bool Unmount(MountId id) = 0;

    /// @brief Returns a read-only snapshot of the active mount table, ordered as resolution sees
    ///        them (longest prefix, then highest priority, then newest mount first).
    [[nodiscard]] virtual std::vector<MountInfo> EnumerateMounts() const = 0;
};

} // namespace Hylux
