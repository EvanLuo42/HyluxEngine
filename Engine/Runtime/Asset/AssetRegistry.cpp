/// @file
/// @brief AssetRegistry implementation. Scan walks every file under the mount root via
///        IVirtualFileSystem::EnumerateFiles, filters for `.hass`, opens each one to
///        read only the 128-byte HassHeader, and extracts the identity tuple. The
///        payload bytes are left on disk — assets stay unloaded until a Load<T> call.

#include "Asset/AssetRegistry.h"

#include "Asset/Cooked/CookedHeader.h"
#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/IVirtualFileSystem.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"

#include <cstring>
#include <string>
#include <vector>

namespace Hylux::Asset
{
namespace
{

constexpr std::string_view kCookedExtension = ".hass";

bool EndsWith(std::string_view str, std::string_view suffix) noexcept
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool ReadHeaderOnly(IFile& file, Cooked::HassHeader& outHeader) noexcept
{
    if (file.Size() < sizeof(Cooked::HassHeader))
    {
        return false;
    }
    return file.Read(&outHeader, sizeof(Cooked::HassHeader)) == sizeof(Cooked::HassHeader);
}

} // namespace

void AssetRegistry::Clear()
{
    entries_.clear();
    pathToGuid_.clear();
}

std::uint32_t AssetRegistry::Scan(IVirtualFileSystem& vfs, std::string_view mountRoot)
{
    std::vector<std::string> candidates;

    vfs.EnumerateFiles(mountRoot, /*recursive*/ true,
                       [&](std::string_view virtualPath, const FileStat&) {
                           if (EndsWith(virtualPath, kCookedExtension))
                           {
                               candidates.emplace_back(virtualPath);
                           }
                       });

    std::uint32_t added = 0;
    for (const auto& path : candidates)
    {
        auto file = vfs.Open(path, FileOpenMode::Read);
        if (!file)
        {
            HYLUX_LOG_WARN(LogAsset, "AssetRegistry: failed to open '{}'", path);
            continue;
        }

        Cooked::HassHeader header{};
        if (!ReadHeaderOnly(*file, header))
        {
            HYLUX_LOG_WARN(LogAsset, "AssetRegistry: '{}' too small for HassHeader", path);
            continue;
        }
        if (header.magic != Cooked::kHassMagic || header.endianTag != Cooked::kHassEndianTag ||
            header.version != Cooked::kHassVersion)
        {
            HYLUX_LOG_WARN(LogAsset, "AssetRegistry: '{}' header rejected (magic=0x{:08x} ver={})",
                           path, header.magic, header.version);
            continue;
        }

        RegistryEntry entry{};
        std::memcpy(entry.guid.bytes.data(), header.guid, Guid::kSize);
        entry.path    = path;
        entry.type    = static_cast<AssetTypeId>(header.typeTag);
        entry.version = header.version;

        if (auto [it, inserted] = entries_.try_emplace(entry.guid, entry); !inserted)
        {
            HYLUX_LOG_WARN(LogAsset,
                           "AssetRegistry: duplicate GUID {} (first at '{}', skipping '{}')",
                           entry.guid.ToString(), it->second.path, path);
            continue;
        }
        pathToGuid_.emplace(entry.path, entry.guid);
        ++added;
    }

    return added;
}

std::optional<RegistryEntry> AssetRegistry::Find(Guid guid) const
{
    if (const auto it = entries_.find(guid); it != entries_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<RegistryEntry> AssetRegistry::FindByPath(std::string_view virtualPath) const
{
    const std::string key{virtualPath};
    if (const auto it = pathToGuid_.find(key); it != pathToGuid_.end())
    {
        if (const auto entryIt = entries_.find(it->second); entryIt != entries_.end())
        {
            return entryIt->second;
        }
    }
    return std::nullopt;
}

AssetRegistry::TypeCounts AssetRegistry::ComputeTypeCounts() const noexcept
{
    TypeCounts counts{};
    for (const auto& [_, entry] : entries_)
    {
        switch (entry.type)
        {
            case AssetTypeId::Mesh:             ++counts.mesh;             break;
            case AssetTypeId::Material:         ++counts.material;         break;
            case AssetTypeId::MaterialInstance: ++counts.materialInstance; break;
            case AssetTypeId::Texture:          ++counts.texture;          break;
            default:                            ++counts.unknown;          break;
        }
    }
    return counts;
}

} // namespace Hylux::Asset
