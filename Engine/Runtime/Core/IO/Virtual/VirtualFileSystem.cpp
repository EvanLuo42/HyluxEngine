/// @file
/// @brief VirtualFileSystem implementation.

#include "Core/IO/Virtual/VirtualFileSystem.h"

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/IFileProvider.h"
#include "Core/IO/Virtual/VirtualPath.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"

#include <algorithm>
#include <cassert>

namespace Hylux
{

VirtualFileSystem::VirtualFileSystem()  = default;
VirtualFileSystem::~VirtualFileSystem() = default;

void VirtualFileSystem::Initialize(Engine& /*engine*/) {}

void VirtualFileSystem::Shutdown()
{
    mounts_.clear();
}

MountId VirtualFileSystem::Mount(std::string_view logicalPrefix,
                                 std::shared_ptr<IFileProvider> provider,
                                 std::int32_t priority)
{
    if (!provider || !VirtualPath::IsValidMountPrefix(logicalPrefix))
    {
        HYLUX_LOG_WARN(LogCore, "VFS::Mount rejected invalid prefix '{}'",
                       std::string(logicalPrefix));
        return MountId::Invalid;
    }

    const MountId id = static_cast<MountId>(nextMountId_.fetch_add(1, std::memory_order_relaxed));

    MountEntry entry;
    entry.id         = id;
    entry.prefix     = std::string(logicalPrefix);
    entry.priority   = priority;
    entry.mountOrder = ++nextMountOrder_;
    entry.provider   = std::move(provider);

    HYLUX_LOG_INFO(LogCore, "VFS::Mount '{}' priority={} provider='{}'",
                   entry.prefix, entry.priority, entry.provider->DebugName());

    mounts_.push_back(std::move(entry));
    Resort();
    return id;
}

bool VirtualFileSystem::Unmount(MountId id)
{
    const auto before = mounts_.size();
    mounts_.erase(std::remove_if(mounts_.begin(), mounts_.end(),
                                 [id](const MountEntry& e) { return e.id == id; }),
                  mounts_.end());
    return mounts_.size() != before;
}

std::vector<MountInfo> VirtualFileSystem::EnumerateMounts() const
{
    std::vector<MountInfo> out;
    out.reserve(mounts_.size());
    for (const auto& m : mounts_)
    {
        out.push_back(MountInfo{m.id, m.prefix, m.priority, m.mountOrder,
                                m.provider ? m.provider->DebugName() : nullptr});
    }
    return out;
}

void VirtualFileSystem::Resort()
{
    std::sort(mounts_.begin(), mounts_.end(),
              [](const MountEntry& a, const MountEntry& b) {
                  if (a.prefix.size() != b.prefix.size())
                      return a.prefix.size() > b.prefix.size();
                  if (a.priority != b.priority) return a.priority > b.priority;
                  return a.mountOrder > b.mountOrder;
              });
}

std::vector<const VirtualFileSystem::MountEntry*>
VirtualFileSystem::CollectMatching(std::string_view normalizedPath,
                                   std::string_view& outSub) const
{
    std::vector<const MountEntry*> out;
    outSub = {};
    for (const auto& m : mounts_)
    {
        if (normalizedPath.size() < m.prefix.size()) continue;
        if (normalizedPath.compare(0, m.prefix.size(), m.prefix) != 0) continue;
        if (out.empty())
        {
            outSub = normalizedPath.substr(m.prefix.size());
        }
        out.push_back(&m);
    }
    return out;
}

const VirtualFileSystem::MountEntry*
VirtualFileSystem::FindWriter(std::string_view normalizedPath, std::string_view& outSub) const
{
    const auto candidates = CollectMatching(normalizedPath, outSub);
    if (candidates.empty()) return nullptr;

    const MountEntry* top = candidates.front();
    if (!top->provider || !top->provider->SupportsWrite()) return nullptr;
    return top;
}

std::unique_ptr<IFile> VirtualFileSystem::Open(std::string_view path, FileOpenMode mode)
{
    const std::string normalized = VirtualPath::Normalize(path);
    if (normalized.empty()) return nullptr;

    const bool wantsWrite = mode == FileOpenMode::Write || mode == FileOpenMode::Append ||
                            mode == FileOpenMode::ReadWrite;

    if (wantsWrite)
    {
        std::string_view sub;
        const MountEntry* writer = FindWriter(normalized, sub);
        if (!writer) return nullptr;
        return writer->provider->Open(sub, mode);
    }

    std::string_view sub;
    const auto candidates = CollectMatching(normalized, sub);
    if (candidates.empty()) return nullptr;

    const std::string_view prefix = candidates.front()->prefix;
    for (const MountEntry* m : candidates)
    {
        if (m->prefix != prefix) break;
        if (!m->provider) continue;
        if (auto handle = m->provider->Open(sub, mode)) return handle;
    }
    return nullptr;
}

bool VirtualFileSystem::Exists(std::string_view path) const
{
    const std::string normalized = VirtualPath::Normalize(path);
    if (normalized.empty()) return false;

    std::string_view sub;
    const auto candidates = CollectMatching(normalized, sub);
    if (candidates.empty()) return false;

    const std::string_view prefix = candidates.front()->prefix;
    for (const MountEntry* m : candidates)
    {
        if (m->prefix != prefix) break;
        if (m->provider && m->provider->Exists(sub)) return true;
    }
    return false;
}

FileStat VirtualFileSystem::Stat(std::string_view path) const
{
    const std::string normalized = VirtualPath::Normalize(path);
    if (normalized.empty()) return {};

    std::string_view sub;
    const auto candidates = CollectMatching(normalized, sub);
    if (candidates.empty()) return {};

    const std::string_view prefix = candidates.front()->prefix;
    for (const MountEntry* m : candidates)
    {
        if (m->prefix != prefix) break;
        if (!m->provider) continue;
        auto stat = m->provider->Stat(sub);
        if (stat.exists) return stat;
    }
    return {};
}

bool VirtualFileSystem::CreateDirectories(std::string_view path)
{
    const std::string normalized = VirtualPath::Normalize(path);
    if (normalized.empty()) return false;

    std::string_view sub;
    const MountEntry* writer = FindWriter(normalized, sub);
    if (!writer) return false;
    return writer->provider->CreateDirectories(sub);
}

bool VirtualFileSystem::Remove(std::string_view path)
{
    const std::string normalized = VirtualPath::Normalize(path);
    if (normalized.empty()) return false;

    std::string_view sub;
    const MountEntry* writer = FindWriter(normalized, sub);
    if (!writer) return false;
    return writer->provider->Remove(sub);
}

bool VirtualFileSystem::Rename(std::string_view /*from*/, std::string_view /*to*/)
{
    return false;
}

bool VirtualFileSystem::IsAbsolute(std::string_view path) const
{
    return !path.empty() && (path.front() == '/' || path.front() == '\\');
}

std::string VirtualFileSystem::JoinPath(std::string_view a, std::string_view b) const
{
    if (a.empty()) return std::string(b);
    if (b.empty()) return std::string(a);

    const bool aEnds   = a.back() == '/' || a.back() == '\\';
    const bool bStarts = b.front() == '/' || b.front() == '\\';

    std::string out;
    out.reserve(a.size() + b.size() + 1);
    out.append(a);
    if (aEnds && bStarts)
        out.append(b.substr(1));
    else if (!aEnds && !bStarts)
    {
        out.push_back('/');
        out.append(b);
    }
    else
        out.append(b);
    return out;
}

std::string VirtualFileSystem::CurrentDirectory() const
{
    return "/";
}

} // namespace Hylux
