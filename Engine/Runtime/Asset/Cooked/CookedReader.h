/// @file
/// @brief CookedReader — reads a .hass file through the VFS, validates the header, and
///        exposes typed pointers into the in-memory buffer for loaders to consume. The
///        reader owns the buffer; pointers / spans returned by its accessors stay valid
///        only as long as the CookedReader instance.

#pragma once

#include "Asset/AssetRef.h"
#include "Asset/AssetTypeId.h"
#include "Asset/Cooked/CookedHeader.h"
#include "Core/Guid.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Hylux
{
class IVirtualFileSystem;
}

namespace Hylux::Asset::Cooked
{

/// @brief Single-shot reader for one cooked .hass file. Construct via Open(); on success
///        the file's bytes are resident in memory and accessible via typed views. Errors
///        in Open (file missing, magic mismatch, version mismatch, truncated, endian
///        mismatch) yield std::nullopt with a LogAsset error.
class CookedReader
{
public:
    CookedReader(const CookedReader&)            = delete;
    CookedReader& operator=(const CookedReader&) = delete;
    CookedReader(CookedReader&&) noexcept        = default;
    CookedReader& operator=(CookedReader&&) noexcept = default;

    [[nodiscard]] static std::optional<CookedReader> Open(IVirtualFileSystem& vfs, std::string_view virtualPath);

    [[nodiscard]] const HassHeader& Header() const noexcept;
    [[nodiscard]] AssetTypeId       TypeTag() const noexcept;
    [[nodiscard]] Guid              GuidValue() const noexcept;

    /// @brief Span over the type-specific payload bytes only (does not include header /
    ///        ref table). Loaders typically forward this to PayloadAs<T>() instead.
    [[nodiscard]] std::span<const std::byte> PayloadBytes() const noexcept;

    /// @brief reinterpret_cast the start of the payload to T*. Caller must know T matches
    ///        the file's typeTag and that sizeof(T) fits within payloadSize.
    template<typename T>
    [[nodiscard]] const T* PayloadAs() const noexcept
    {
        return reinterpret_cast<const T*>(PayloadBytes().data());
    }

    /// @brief Returns the number of cross-asset references encoded in this file.
    [[nodiscard]] std::uint32_t RefCount() const noexcept;

    /// @brief Resolves one ref-table entry into a SoftAssetRef (guid + path hint).
    ///        Returns an invalid (zero-guid) ref when index is out of bounds.
    [[nodiscard]] SoftAssetRef Ref(std::uint32_t index) const;

    /// @brief Returns a pointer to a value of type T located at the given file-relative
    ///        offset. Bounds-checks that offset + sizeof(T) fits in the buffer; on miss
    ///        returns nullptr. Loaders use this for payload sub-sections (vertex layout,
    ///        submeshes, parameter entries, mip table).
    template<typename T>
    [[nodiscard]] const T* At(std::uint32_t offset) const noexcept
    {
        return InBounds(offset, sizeof(T)) ? reinterpret_cast<const T*>(BufferAt(offset)) : nullptr;
    }

    /// @brief Returns a span of `count` Ts starting at the file-relative offset. Returns
    ///        an empty span when out of bounds.
    template<typename T>
    [[nodiscard]] std::span<const T> Range(std::uint32_t offset, std::uint32_t count) const noexcept
    {
        const std::uint64_t totalBytes = static_cast<std::uint64_t>(count) * sizeof(T);
        if (totalBytes > 0xFFFFFFFFull || !InBounds(offset, static_cast<std::uint32_t>(totalBytes)))
        {
            return {};
        }
        return std::span<const T>(reinterpret_cast<const T*>(BufferAt(offset)), count);
    }

    /// @brief Returns a span of raw bytes starting at offset for `size` bytes. Use for
    ///        vertex / index / pixel bulk payloads where the type is the caller's concern.
    [[nodiscard]] std::span<const std::byte> ByteRange(std::uint32_t offset, std::uint32_t size) const noexcept;

private:
    CookedReader() = default;

    [[nodiscard]] bool         InBounds(std::uint32_t offset, std::uint32_t size) const noexcept;
    [[nodiscard]] const std::byte* BufferAt(std::uint32_t offset) const noexcept;

    std::vector<std::byte> buffer_;
};

} // namespace Hylux::Asset::Cooked
