/// @file
/// @brief MaterialInstanceLoader implementation. Recurses asynchronously for the parent
///        material via AssetSubsystem::LoadGenericByGuid; chains the instance build on
///        the parent's Future.

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
namespace
{

MaterialInstanceAsset::InitData BuildOverrides(const Cooked::CookedReader&            reader,
                                                const Cooked::MaterialInstancePayload& payload)
{
    MaterialInstanceAsset::InitData init{};
    auto overrideEntries =
        reader.Range<Cooked::OverrideEntry>(payload.overrideOffset, payload.overrideCount);
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
    return init;
}

} // namespace

Future<Ref<MaterialInstanceAsset>> MaterialInstanceLoader::Load(const Cooked::CookedReader& reader,
                                                                const AssetLoaderContext&    ctx)
{
    if (reader.TypeTag() != AssetTypeId::MaterialInstance)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialInstanceLoader: wrong type tag {}",
                        static_cast<int>(reader.TypeTag()));
        return Future<Ref<MaterialInstanceAsset>>::MakeFailed();
    }

    const auto* payload = reader.PayloadAs<Cooked::MaterialInstancePayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialInstanceLoader: payload missing");
        return Future<Ref<MaterialInstanceAsset>>::MakeFailed();
    }

    MaterialInstanceAsset::InitData init = BuildOverrides(reader, *payload);
    AssetId                          id{reader.GuidValue(), {}};

    const bool hasParentRef = payload->parentRefIndex < reader.RefCount() && ctx.subsystem != nullptr;
    if (!hasParentRef)
    {
        Ref<MaterialInstanceAsset> asset = MakeRef<MaterialInstanceAsset>(std::move(id), std::move(init));
        return Future<Ref<MaterialInstanceAsset>>::MakeReady(std::move(asset));
    }

    const SoftAssetRef     parentRef    = reader.Ref(payload->parentRefIndex);
    Future<Ref<AssetBase>> parentFuture = ctx.subsystem->LoadGenericByGuid(parentRef.guid);
    Guid                   parentGuid   = parentRef.guid;

    return parentFuture.Transform<Ref<MaterialInstanceAsset>>(
        [init = std::move(init), id = std::move(id), parentGuid](Ref<AssetBase>& parent) mutable
            -> Ref<MaterialInstanceAsset> {
            if (parent && parent->GetAssetTypeId() == AssetTypeId::Material)
            {
                init.parent = Ref<MaterialAsset>(static_cast<MaterialAsset*>(parent.Get()));
            }
            else
            {
                HYLUX_LOG_WARN(LogAsset,
                               "MaterialInstanceLoader: parent guid {} not resolvable to a MaterialAsset",
                               parentGuid.ToString());
            }
            return MakeRef<MaterialInstanceAsset>(std::move(id), std::move(init));
        });
}

} // namespace Hylux::Asset
