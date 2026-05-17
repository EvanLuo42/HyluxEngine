/// @file
/// @brief On-disk persistence for IRHIPipelineCache blobs. Backed by a flat filesystem
///        directory: one `.psoblob` per cache name. Engine convention is to use a single
///        directory configured via RendererConfig::psoCacheDir, never mounted into the
///        virtual filesystem (per the project's shader-cache isolation rule).

#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Loads / stores opaque pipeline-cache blobs to a fixed directory.
class PsoBlobStore
{
public:
    explicit PsoBlobStore(std::filesystem::path directory);

    PsoBlobStore(const PsoBlobStore&)            = delete;
    PsoBlobStore& operator=(const PsoBlobStore&) = delete;
    PsoBlobStore(PsoBlobStore&&)                 = delete;
    PsoBlobStore& operator=(PsoBlobStore&&)      = delete;

    [[nodiscard]] const std::filesystem::path& GetDirectory() const noexcept { return directory_; }

    /// @brief Returns the blob bytes for the given name, or an empty vector when the file
    ///        is missing or unreadable.
    [[nodiscard]] std::vector<std::byte> Load(std::string_view name) const;

    /// @brief Persists the supplied bytes under the given name. Writes to a temp file
    ///        first and then renames into place so partial writes never replace a good blob.
    bool Save(std::string_view name, std::span<const std::byte> blob);

private:
    [[nodiscard]] std::filesystem::path BuildPath(std::string_view name) const;

    std::filesystem::path directory_;
};

} // namespace Hylux::Renderer
