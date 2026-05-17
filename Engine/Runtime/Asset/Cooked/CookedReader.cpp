/// @file
/// @brief CookedReader implementation. Reads the file fully into a heap buffer (v1 does
///        not mmap; the VFS does not yet expose mapped views), validates the header, and
///        keeps the buffer alive so reinterpret_cast accessors stay valid.

#include "Asset/Cooked/CookedReader.h"

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/IVirtualFileSystem.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"

#include <cstring>
#include <utility>

namespace Hylux::Asset::Cooked
{

namespace
{

bool ReadAll(IFile& file, std::vector<std::byte>& out)
{
    const FileSize total = file.Size();
    if (total == 0)
    {
        return false;
    }
    out.resize(static_cast<std::size_t>(total));
    const FileSize read = file.Read(out.data(), total);
    return read == total;
}

} // namespace

std::optional<CookedReader> CookedReader::Open(IVirtualFileSystem& vfs, std::string_view virtualPath)
{
    auto file = vfs.Open(virtualPath, FileOpenMode::Read);
    if (!file)
    {
        return std::nullopt;
    }

    CookedReader reader;
    if (!ReadAll(*file, reader.buffer_))
    {
        HYLUX_LOG_ERROR(LogAsset, "CookedReader: failed to read full file '{}'", virtualPath);
        return std::nullopt;
    }

    if (reader.buffer_.size() < sizeof(HassHeader))
    {
        HYLUX_LOG_ERROR(LogAsset, "CookedReader: '{}' is smaller than HassHeader ({} bytes)",
                        virtualPath, reader.buffer_.size());
        return std::nullopt;
    }

    const auto& header = reader.Header();
    if (header.magic != kHassMagic)
    {
        HYLUX_LOG_ERROR(LogAsset, "CookedReader: '{}' bad magic 0x{:08x}", virtualPath, header.magic);
        return std::nullopt;
    }
    if (header.endianTag != kHassEndianTag)
    {
        HYLUX_LOG_ERROR(LogAsset, "CookedReader: '{}' endian mismatch (tag=0x{:08x})", virtualPath,
                        header.endianTag);
        return std::nullopt;
    }
    if (header.version != kHassVersion)
    {
        HYLUX_LOG_ERROR(LogAsset, "CookedReader: '{}' version {} not supported (expected {})",
                        virtualPath, header.version, kHassVersion);
        return std::nullopt;
    }

    const std::uint64_t payloadEnd =
        static_cast<std::uint64_t>(header.payloadOffset) + header.payloadSize;
    if (payloadEnd > reader.buffer_.size())
    {
        HYLUX_LOG_ERROR(LogAsset,
                        "CookedReader: '{}' payload [{}, {}) exceeds file size {}",
                        virtualPath, header.payloadOffset, payloadEnd, reader.buffer_.size());
        return std::nullopt;
    }

    if (header.refTableCount > 0)
    {
        const std::uint64_t refEntriesEnd =
            static_cast<std::uint64_t>(header.refTableOffset) +
            static_cast<std::uint64_t>(header.refTableCount) * sizeof(RefEntry);
        if (refEntriesEnd > reader.buffer_.size() || header.refTableOffset > reader.buffer_.size())
        {
            HYLUX_LOG_ERROR(LogAsset,
                            "CookedReader: '{}' ref-table [{}, {}) exceeds file size {}",
                            virtualPath, header.refTableOffset, refEntriesEnd, reader.buffer_.size());
            return std::nullopt;
        }
    }

    return reader;
}

const HassHeader& CookedReader::Header() const noexcept
{
    return *reinterpret_cast<const HassHeader*>(buffer_.data());
}

AssetTypeId CookedReader::TypeTag() const noexcept
{
    return static_cast<AssetTypeId>(Header().typeTag);
}

Guid CookedReader::GuidValue() const noexcept
{
    Guid g;
    std::memcpy(g.bytes.data(), Header().guid, Guid::kSize);
    return g;
}

std::span<const std::byte> CookedReader::PayloadBytes() const noexcept
{
    const auto& h = Header();
    return ByteRange(h.payloadOffset, h.payloadSize);
}

std::uint32_t CookedReader::RefCount() const noexcept
{
    return Header().refTableCount;
}

SoftAssetRef CookedReader::Ref(std::uint32_t index) const
{
    const auto& h = Header();
    if (index >= h.refTableCount)
    {
        return {};
    }

    const std::uint32_t entryOffset =
        h.refTableOffset + static_cast<std::uint32_t>(index * sizeof(RefEntry));
    const auto* entry = At<RefEntry>(entryOffset);
    if (entry == nullptr)
    {
        return {};
    }

    Guid g;
    std::memcpy(g.bytes.data(), entry->guid, Guid::kSize);

    std::string pathHint;
    if (entry->pathLength > 0)
    {
        const std::uint32_t poolStart =
            h.refTableOffset + h.refTableCount * static_cast<std::uint32_t>(sizeof(RefEntry));
        const std::uint32_t absolute = poolStart + entry->pathOffset;
        if (InBounds(absolute, entry->pathLength))
        {
            pathHint.assign(reinterpret_cast<const char*>(BufferAt(absolute)), entry->pathLength);
        }
    }
    return SoftAssetRef{g, std::move(pathHint)};
}

std::span<const std::byte> CookedReader::ByteRange(std::uint32_t offset, std::uint32_t size) const noexcept
{
    if (!InBounds(offset, size))
    {
        return {};
    }
    return std::span<const std::byte>(BufferAt(offset), size);
}

bool CookedReader::InBounds(std::uint32_t offset, std::uint32_t size) const noexcept
{
    const std::uint64_t end = static_cast<std::uint64_t>(offset) + size;
    return end <= buffer_.size();
}

const std::byte* CookedReader::BufferAt(std::uint32_t offset) const noexcept
{
    return buffer_.data() + offset;
}

} // namespace Hylux::Asset::Cooked
