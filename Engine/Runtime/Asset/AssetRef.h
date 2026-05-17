/// @file
/// @brief SoftAssetRef — late-bound reference to another asset. Holds the target's GUID
///        and the original path (as a debugging hint that survives renames). Resolution
///        is deferred to an explicit Resolve() call that goes through AssetSubsystem.

#pragma once

#include "Core/Guid.h"

#include <string>
#include <utility>

namespace Hylux::Asset
{

/// @brief Untyped soft reference. Stored inside other asset payloads (e.g. a
///        MaterialAsset's texture default points at a TextureAsset via SoftAssetRef).
///        Type-erased so RefTable encoding stays uniform; the consuming loader knows
///        the expected type and casts after Resolve().
struct SoftAssetRef
{
    Guid        guid{};
    std::string pathHint{};

    SoftAssetRef() = default;
    SoftAssetRef(Guid g, std::string p) noexcept : guid(g), pathHint(std::move(p)) {}

    [[nodiscard]] bool IsValid() const noexcept { return !guid.IsZero(); }
};

} // namespace Hylux::Asset
