/// @file
/// @brief AssetBase — common refcounted base for every runtime asset type. Carries the
///        canonical AssetId and the virtual GetAssetTypeId / GetMemoryFootprint pair the
///        subsystem and LRU need.

#pragma once

#include "Asset/AssetId.h"
#include "Asset/AssetTypeId.h"
#include "Core/Memory/RefCounted.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

/// @brief Refcounted base for every asset type. Subclasses report their type tag and
///        an approximate GPU+CPU memory footprint so the AssetSubsystem's LRU can size
///        its budget correctly.
class AssetBase : public Hylux::RefCounted
{
public:
    explicit AssetBase(AssetId id) noexcept : id_(std::move(id)) {}

    [[nodiscard]] const AssetId& GetId() const noexcept { return id_; }
    [[nodiscard]] const Guid&    GetGuid() const noexcept { return id_.guid; }

    [[nodiscard]] virtual AssetTypeId   GetAssetTypeId() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetMemoryFootprint() const noexcept = 0;

protected:
    ~AssetBase() override = default;

private:
    AssetId id_;
};

} // namespace Hylux::Asset
