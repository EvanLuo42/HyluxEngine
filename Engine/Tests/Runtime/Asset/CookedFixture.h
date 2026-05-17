/// @file
/// @brief Tiny helper that builds synthetic .hass-style byte buffers (header + payload +
///        ref table) for CookedReader / AssetRegistry tests.

#pragma once

#include "Asset/AssetTypeId.h"
#include "Asset/Cooked/CookedHeader.h"
#include "Core/Guid.h"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace Hylux::Tests
{

struct RefDesc
{
    Guid      guid{};
    std::string_view pathHint{};
};

/// @brief Builds a buffer containing a HassHeader followed by `payload` followed by an
///        optional ref table + path-string pool. The returned buffer can be written
///        verbatim to a file and reopened with CookedReader::Open.
inline std::vector<std::uint8_t> BuildCookedFile(Asset::AssetTypeId       type,
                                                 Guid              guid,
                                                 std::string_view         payload,
                                                 std::vector<RefDesc>     refs = {},
                                                 std::uint32_t            version = Asset::Cooked::kHassVersion,
                                                 std::uint32_t            magic = Asset::Cooked::kHassMagic,
                                                 std::uint32_t            endianTag = Asset::Cooked::kHassEndianTag)
{
    constexpr std::uint32_t headerSize  = sizeof(Asset::Cooked::HassHeader);
    const std::uint32_t     payloadOff  = headerSize;
    const std::uint32_t     payloadSize = static_cast<std::uint32_t>(payload.size());
    const std::uint32_t     refTableOff = payloadOff + payloadSize;
    const std::uint32_t     refCount    = static_cast<std::uint32_t>(refs.size());
    const std::uint32_t     refTableBytes = refCount * static_cast<std::uint32_t>(sizeof(Asset::Cooked::RefEntry));

    // Path pool starts right after the ref entries.
    std::vector<std::uint32_t> pathOffsets(refs.size(), 0u);
    std::uint32_t poolBytes = 0;
    for (std::size_t i = 0; i < refs.size(); ++i)
    {
        pathOffsets[i] = poolBytes;
        poolBytes += static_cast<std::uint32_t>(refs[i].pathHint.size());
    }

    const std::uint32_t total = refTableOff + refTableBytes + poolBytes;
    std::vector<std::uint8_t> buf(total, 0u);

    Asset::Cooked::HassHeader header{};
    header.magic           = magic;
    header.endianTag       = endianTag;
    header.version         = static_cast<std::uint16_t>(version);
    header.typeTag         = static_cast<std::uint16_t>(type);
    std::memcpy(header.guid, guid.bytes.data(), Guid::kSize);
    header.payloadOffset   = payloadOff;
    header.payloadSize     = payloadSize;
    header.refTableOffset  = refCount > 0 ? refTableOff : 0u;
    header.refTableCount   = refCount;
    std::memcpy(buf.data(), &header, sizeof(header));

    std::memcpy(buf.data() + payloadOff, payload.data(), payload.size());

    for (std::size_t i = 0; i < refs.size(); ++i)
    {
        Asset::Cooked::RefEntry entry{};
        std::memcpy(entry.guid, refs[i].guid.bytes.data(), Guid::kSize);
        entry.pathOffset = pathOffsets[i];
        entry.pathLength = static_cast<std::uint32_t>(refs[i].pathHint.size());
        std::memcpy(buf.data() + refTableOff + i * sizeof(Asset::Cooked::RefEntry), &entry, sizeof(entry));
    }

    const std::uint32_t poolStart = refTableOff + refTableBytes;
    for (std::size_t i = 0; i < refs.size(); ++i)
    {
        const auto& r = refs[i];
        if (!r.pathHint.empty())
        {
            std::memcpy(buf.data() + poolStart + pathOffsets[i], r.pathHint.data(), r.pathHint.size());
        }
    }

    return buf;
}

/// @brief Same as BuildCookedFile but corrupts the header field selected by @p which.
enum class HeaderCorruption
{
    Magic,
    Endian,
    Version,
    PayloadOverrun,
    RefTableOverrun,
};

inline std::vector<std::uint8_t> BuildCorruptCookedFile(HeaderCorruption which)
{
    auto buf = BuildCookedFile(Asset::AssetTypeId::Mesh, Guid::Generate(), "p");
    auto* header = reinterpret_cast<Asset::Cooked::HassHeader*>(buf.data());
    switch (which)
    {
        case HeaderCorruption::Magic:   header->magic = 0xDEADBEEFu; break;
        case HeaderCorruption::Endian:  header->endianTag = 0x04030201u; break;
        case HeaderCorruption::Version: header->version = 99; break;
        case HeaderCorruption::PayloadOverrun:
            header->payloadOffset = static_cast<std::uint32_t>(buf.size() - 1);
            header->payloadSize   = 9999;
            break;
        case HeaderCorruption::RefTableOverrun:
            header->refTableCount  = 5;
            header->refTableOffset = static_cast<std::uint32_t>(buf.size() - 1);
            break;
    }
    return buf;
}

} // namespace Hylux::Tests
