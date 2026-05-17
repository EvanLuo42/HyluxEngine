/// @file
/// @brief MaterialInstanceLoader implementation. Recurses for the parent material via
///        AssetSubsystem::LoadGenericByGuid (which itself hits the LRU on warm).

#include "Asset/Loaders/MaterialInstanceLoader.h"

#include "Asset/AssetSubsystem.h"
#include "Asset/Cooked/CookedReader.h"
#include "Asset/Types/MaterialAsset.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"

#include <cstring>

namespace Hylux::Asset
{

Ref<MaterialInstanceAsset> MaterialInstanceLoader::Load(const Cooked::CookedReader& reader,
                                                        const AssetLoaderContext&    ctx)
{
    if (reader.TypeTag() != AssetTypeId::MaterialInstance)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialInstanceLoader: wrong type tag {}",
                        static_cast<int>(reader.TypeTag()));
        return {};
    }

    const auto* payload = reader.PayloadAs<Cooked::MaterialInstancePayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialInstanceLoader: payload missing");
        return {};
    }

    MaterialInstanceAsset::InitData init{};

    if (payload->parentRefIndex < reader.RefCount() && ctx.subsystem != nullptr)
    {
        const SoftAssetRef parentRef = reader.Ref(payload->parentRefIndex);
        Ref<AssetBase>     parent    = ctx.subsystem->LoadGenericByGuid(parentRef.guid).Wait();
        if (parent && parent->GetAssetTypeId() == AssetTypeId::Material)
        {
            init.parent = Ref<MaterialAsset>(static_cast<MaterialAsset*>(parent.Get()));
        }
        else
        {
            HYLUX_LOG_WARN(LogAsset,
                           "MaterialInstanceLoader: parent guid {} not resolvable to a MaterialAsset",
                           parentRef.guid.ToString());
        }
    }

    auto overrideEntries =
        reader.Range<Cooked::OverrideEntry>(payload->overrideOffset, payload->overrideCount);
    init.overrides.reserve(overrideEntries.size());
    for (const auto& e : overrideEntries)
    {
        MaterialInstanceAsset::ParameterOverride ov{};
        ov.name       = static_cast<NameHash>(e.nameHash);
        ov.value.kind = static_cast<ParameterKind>(e.kind);
        std::memcpy(ov.value.vec, e.valueBytes, sizeof(ov.value.vec));
        if (ov.value.kind == ParameterKind::Texture && e.refIndex < reader.RefCount())
        {
            ov.textureRef = reader.Ref(e.refIndex);
        }
        init.overrides.push_back(std::move(ov));
    }

    AssetId id{reader.GuidValue(), {}};
    return MakeRef<MaterialInstanceAsset>(std::move(id), std::move(init));
}

} // namespace Hylux::Asset
