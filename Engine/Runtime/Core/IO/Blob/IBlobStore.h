/// @file
/// @brief Abstract read-only store for opaque binary blobs keyed by a logical name.
///        Backed by a filesystem directory in editor builds and (later) by a VFS pak
///        provider in shipped game builds. Distinct from IFileSystem: IBlobStore is
///        mmap-aware and presents whole-file byte ranges instead of streamable handles.

#pragma once

#include "Core/IO/Blob/IMappedBlob.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace Hylux
{

/// @brief Read-only blob lookup. Each call to OpenMapped returns a fresh IMappedBlob
///        whose bytes remain valid until the returned handle is destroyed. Backends are
///        free to mmap, heap-load, or even satisfy from an in-memory cache; the consumer
///        treats the result uniformly. Implementations are not required to be thread-safe
///        unless documented.
class IBlobStore
{
public:
    virtual ~IBlobStore() = default;

    IBlobStore(const IBlobStore&)            = delete;
    IBlobStore& operator=(const IBlobStore&) = delete;
    IBlobStore(IBlobStore&&)                 = delete;
    IBlobStore& operator=(IBlobStore&&)      = delete;

    /// @brief Opens the blob identified by @p logicalKey. Returns nullptr if the key is
    ///        unknown or the blob cannot be opened. The returned handle owns its mapping.
    [[nodiscard]] virtual std::unique_ptr<IMappedBlob> OpenMapped(std::string_view logicalKey) = 0;

    /// @brief Returns true if a blob with the given key is currently visible to the store.
    [[nodiscard]] virtual bool Exists(std::string_view logicalKey) const = 0;

    /// @brief Returns the last-modified timestamp for the blob, or std::nullopt if the
    ///        backend does not expose timestamps or the key is unknown. Used by the
    ///        shader subsystem's hot-reload watcher.
    [[nodiscard]] virtual std::optional<std::filesystem::file_time_type>
        LastModified(std::string_view logicalKey) const = 0;

    /// @brief Returns a diagnostic identifier for the store as a whole (e.g. "filesystem:
    ///        C:/Users/.../ShaderCache"). Used only for logging.
    [[nodiscard]] virtual std::string_view DebugName() const noexcept = 0;

protected:
    IBlobStore() = default;
};

} // namespace Hylux
