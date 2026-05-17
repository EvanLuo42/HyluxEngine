/// @file
/// @brief Saved material-instance asset — a set of parameter overrides on top of a
///        parent MaterialAsset that artists author in the editor. Distinct from
///        Renderer::MaterialInstance (the per-entity mutable runtime override container);
///        a runtime MaterialInstance may be seeded from a MaterialInstanceAsset and then
///        diverge with per-entity tweaks.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetRef.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/MaterialParameter.h"
#include "Core/Memory/Ref.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Hylux::Asset
{

class TextureAsset;

class MaterialInstanceAsset final : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::MaterialInstance;

    struct ParameterOverride
    {
        NameHash       name{NameHash::Invalid};
        ParameterValue value{};
        SoftAssetRef   textureRef{};
    };

    struct InitData
    {
        Ref<MaterialAsset>             parent;
        std::vector<ParameterOverride> overrides{};
    };

    MaterialInstanceAsset(AssetId id, InitData data) noexcept
        : AssetBase(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] AssetTypeId  GetAssetTypeId() const noexcept override { return AssetTypeId::MaterialInstance; }
    [[nodiscard]] std::uint64_t GetMemoryFootprint() const noexcept override { return 0; }

    [[nodiscard]] MaterialAsset*                          GetParent() const noexcept { return data_.parent.Get(); }
    [[nodiscard]] const std::vector<ParameterOverride>&   GetOverrides() const noexcept { return data_.overrides; }

private:
    ~MaterialInstanceAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
