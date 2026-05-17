/// @file
/// @brief ShaderManifest implementation: text-based sidecar format keyed on relative
///        source path. Format is line-oriented and tolerant of trailing newlines so it
///        diffs cleanly under version control if anyone wants to inspect it.

#include "ShaderCompiler/ShaderManifest.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Utils/Hash.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>
#include <system_error>

namespace Hylux::Editor
{
namespace
{

constexpr std::string_view kFormatHeader = "hylux-shader-manifest";

[[nodiscard]] std::uint64_t HashFileContents(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return 0;
    }
    std::uint64_t        seed = Hash::Fnv1a64Offset;
    constexpr std::size_t kChunkBytes = 64 * 1024;
    std::vector<char>    buffer(kChunkBytes);
    while (stream)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize got = stream.gcount();
        if (got <= 0)
        {
            break;
        }
        seed = Hash::MixBytes(seed, buffer.data(), static_cast<std::size_t>(got));
    }
    return seed;
}

[[nodiscard]] std::int64_t LastWriteNs(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto      ft = std::filesystem::last_write_time(path, ec);
    if (ec)
    {
        return 0;
    }
    const auto sinceEpoch = ft.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(sinceEpoch).count();
}

} // namespace

void ShaderManifest::SetEntries(std::vector<ShaderManifestEntry> entries)
{
    std::sort(entries.begin(), entries.end(),
              [](const ShaderManifestEntry& a, const ShaderManifestEntry& b) { return a.relativePath < b.relativePath; });
    entries_ = std::move(entries);
}

bool ShaderManifest::MatchesContent(const ShaderManifest& other) const noexcept
{
    if (entries_.size() != other.entries_.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < entries_.size(); ++i)
    {
        if (entries_[i].relativePath != other.entries_[i].relativePath ||
            entries_[i].contentHash != other.entries_[i].contentHash)
        {
            return false;
        }
    }
    return true;
}

bool ShaderManifest::Save(const std::filesystem::path& path) const
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream stream(path, std::ios::trunc);
    if (!stream)
    {
        HYLUX_LOG_WARN(LogRender, "ShaderManifest: cannot write '{}'", path.string());
        return false;
    }
    stream << kFormatHeader << ' ' << version_ << '\n';
    for (const auto& entry : entries_)
    {
        stream << entry.contentHash << ' ' << entry.lastWriteEpochNs << ' ' << entry.relativePath << '\n';
    }
    return static_cast<bool>(stream);
}

bool ShaderManifest::Load(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    if (!stream)
    {
        return false;
    }
    std::string header;
    std::uint32_t version = 0;
    stream >> header >> version;
    if (header != kFormatHeader || version != kCurrentVersion)
    {
        return false;
    }
    std::vector<ShaderManifestEntry> entries;
    std::uint64_t                    contentHash = 0;
    std::int64_t                     epochNs     = 0;
    std::string                      tail;
    while (stream >> contentHash >> epochNs)
    {
        std::getline(stream, tail);
        std::string_view view(tail);
        while (!view.empty() && (view.front() == ' ' || view.front() == '\t'))
        {
            view.remove_prefix(1);
        }
        entries.push_back(ShaderManifestEntry{std::string(view), contentHash, epochNs});
    }
    version_ = version;
    SetEntries(std::move(entries));
    return true;
}

ShaderManifest ShaderManifest::BuildFromSourceDir(const std::filesystem::path& sourceDir)
{
    ShaderManifest manifest;
    if (!std::filesystem::is_directory(sourceDir))
    {
        return manifest;
    }
    std::vector<ShaderManifestEntry> entries;
    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(sourceDir))
    {
        if (!dirEntry.is_regular_file())
        {
            continue;
        }
        if (dirEntry.path().extension() != ".slang")
        {
            continue;
        }
        ShaderManifestEntry e{};
        e.relativePath     = std::filesystem::relative(dirEntry.path(), sourceDir).generic_string();
        e.contentHash      = HashFileContents(dirEntry.path());
        e.lastWriteEpochNs = LastWriteNs(dirEntry.path());
        entries.push_back(std::move(e));
    }
    manifest.SetEntries(std::move(entries));
    return manifest;
}

} // namespace Hylux::Editor
