/// @file
/// @brief AssetId — the (guid, path) value used to identify an asset throughout the
///        engine. The guid is canonical; the path is a human-readable hint that survives
///        in logs and on disk but is not load-bearing.

#pragma once

#include "Asset/Guid.h"

#include <string>
#include <string_view>
#include <utility>

namespace Hylux::Asset
{

struct AssetId
{
    Guid        guid{};
    std::string path{};

    AssetId() = default;
    AssetId(Guid g, std::string p) noexcept : guid(g), path(std::move(p)) {}

    [[nodiscard]] bool IsValid() const noexcept { return !guid.IsZero(); }
};

[[nodiscard]] inline bool operator==(const AssetId& a, const AssetId& b) noexcept
{
    return a.guid == b.guid;
}

[[nodiscard]] inline bool operator!=(const AssetId& a, const AssetId& b) noexcept
{
    return !(a == b);
}

} // namespace Hylux::Asset
