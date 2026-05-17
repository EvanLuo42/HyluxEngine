/// @file
/// @brief MaterialLoader implementation.

#include "Asset/Loaders/MaterialLoader.h"

#include "Asset/AssetRef.h"
#include "Asset/Cooked/CookedReader.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "Shader/ShaderSubsystem.h"

#include <cstring>

namespace Hylux::Asset
{

Ref<MaterialAsset> MaterialLoader::Load(const Cooked::CookedReader& reader,
                                        const AssetLoaderContext&    ctx)
{
    if (reader.TypeTag() != AssetTypeId::Material)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialLoader: wrong type tag {}",
                        static_cast<int>(reader.TypeTag()));
        return {};
    }

    const auto* payload = reader.PayloadAs<Cooked::MaterialPayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "MaterialLoader: payload missing");
        return {};
    }

    MaterialAsset::InitData init{};
    init.shaderMapHash      = payload->shaderMapHash;
    init.permutationBaseKey = payload->permutationBaseKey;
    init.uniformBlockSize   = payload->uniformBlockSize;
    init.textureCount       = payload->textureCount;

    auto entries = reader.Range<Cooked::ParameterEntry>(payload->parameterOffset, payload->parameterCount);
    init.parameters.reserve(entries.size());
    for (const auto& e : entries)
    {
        ParameterDesc desc{};
        desc.name           = static_cast<NameHash>(e.nameHash);
        desc.kind           = static_cast<ParameterKind>(e.kind);
        desc.isPermutation  = e.permutationContrib != 0;
        if (desc.kind == ParameterKind::Texture)
        {
            desc.textureSlot = e.uniformOffsetOrTextureSlot;
        }
        else
        {
            desc.uniformOffset = e.uniformOffsetOrTextureSlot;
        }
        init.parameters.push_back(desc);
    }

    init.defaultTextures.reserve(reader.RefCount());
    for (std::uint32_t i = 0; i < reader.RefCount(); ++i)
    {
        init.defaultTextures.push_back(reader.Ref(i));
    }

    if (ctx.shaders != nullptr && init.shaderMapHash != 0)
    {
        init.shaderMap = ctx.shaders->GetOrLoadMaterialShaderMap(init.shaderMapHash);
    }

    AssetId id{reader.GuidValue(), {}};
    return MakeRef<MaterialAsset>(std::move(id), std::move(init));
}

} // namespace Hylux::Asset
