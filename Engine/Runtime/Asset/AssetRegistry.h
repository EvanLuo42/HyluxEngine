/// @file
/// @brief AssetRegistry — GUID → (path, type) map populated by walking the cooked tree
///        at AssetSubsystem::Initialize. The runtime never reads source JSON; the
///        registry sees only `.hass` files. Used to resolve GUID-keyed cross-asset
///        references back to the path their loader must open.

#pragma once

#include "Asset/AssetId.h"
#include "Asset/AssetTypeId.h"
#include "Core/Guid.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hylux
{
class IVirtualFileSystem;
}

namespace Hylux::Asset
{

struct RegistryEntry
{
    Guid          guid{};
    std::string   path{};
    AssetTypeId   type{AssetTypeId::Unknown};
    std::uint16_t version{0};
};

/// @brief Per-AssetSubsystem registry. Single-threaded by design (the AssetSubsystem
///        only mutates from the game thread during Initialize / hot reload).
class AssetRegistry
{
public:
    AssetRegistry() = default;

    AssetRegistry(const AssetRegistry&)            = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    /// @brief Discards every entry. Called at Shutdown and before a fresh full scan.
    void Clear();

    /// @brief Walks `mountRoot` (a VFS prefix like "/Engine/" or "/Game/") for `.hass`
    ///        files, reads each file's 128-byte HassHeader, and inserts the (guid, path,
    ///        type, version) tuple into the map. Duplicate GUIDs log a warning and the
    ///        first-seen entry wins. Returns the number of entries added.
    std::uint32_t Scan(IVirtualFileSystem& vfs, std::string_view mountRoot);

    /// @brief Looks up by GUID. Returns nullopt if absent.
    [[nodiscard]] std::optional<RegistryEntry> Find(Guid guid) const;

    /// @brief Looks up by VFS path. Returns nullopt if absent.
    [[nodiscard]] std::optional<RegistryEntry> FindByPath(std::string_view virtualPath) const;

    [[nodiscard]] std::size_t                  Size() const noexcept { return entries_.size(); }
    [[nodiscard]] const std::unordered_map<Guid, RegistryEntry>& Entries() const noexcept { return entries_; }

    /// @brief Per-type counts for diagnostics / log lines.
    struct TypeCounts
    {
        std::uint32_t mesh{0};
        std::uint32_t material{0};
        std::uint32_t materialInstance{0};
        std::uint32_t texture{0};
        std::uint32_t unknown{0};
    };
    [[nodiscard]] TypeCounts ComputeTypeCounts() const noexcept;

private:
    std::unordered_map<Guid, RegistryEntry>          entries_;
    std::unordered_map<std::string, Guid>            pathToGuid_;
};

} // namespace Hylux::Asset
