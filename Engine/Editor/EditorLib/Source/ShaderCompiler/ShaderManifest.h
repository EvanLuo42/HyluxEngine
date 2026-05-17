/// @file
/// @brief Sidecar manifest written next to the .hslib archive. Records the source files
///        that fed the last successful build along with their content hashes so a
///        subsequent launch can skip compilation when nothing has changed.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Hylux::Editor
{

/// @brief One entry per source file participating in the archive.
struct ShaderManifestEntry
{
    std::string   relativePath;
    std::uint64_t contentHash{0};
    std::int64_t  lastWriteEpochNs{0};
};

/// @brief Versioned manifest. The compiler writes one on every successful build and
///        compares against the previous one to decide whether to recompile.
class ShaderManifest
{
public:
    static constexpr std::uint32_t kCurrentVersion = 1;

    [[nodiscard]] std::uint32_t                            Version() const noexcept { return version_; }
    [[nodiscard]] const std::vector<ShaderManifestEntry>&  Entries() const noexcept { return entries_; }

    void SetEntries(std::vector<ShaderManifestEntry> entries);

    /// @brief True when the entry sets are equal in content (path + content hash).
    [[nodiscard]] bool MatchesContent(const ShaderManifest& other) const noexcept;

    [[nodiscard]] bool Save(const std::filesystem::path& path) const;
    [[nodiscard]] bool Load(const std::filesystem::path& path);

    /// @brief Walks sourceDir for .slang files, hashing each. Used both before compilation
    ///        (to compare against a saved manifest) and after (to persist the result).
    [[nodiscard]] static ShaderManifest BuildFromSourceDir(const std::filesystem::path& sourceDir);

private:
    std::uint32_t                    version_{kCurrentVersion};
    std::vector<ShaderManifestEntry> entries_;
};

} // namespace Hylux::Editor
