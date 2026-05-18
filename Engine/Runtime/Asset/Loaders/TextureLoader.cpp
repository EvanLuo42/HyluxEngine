/// @file
/// @brief TextureLoader implementation. Dispatches on Cooked::TexturePayload::kind to
///        produce a PhysicalTextureAsset (kind = Physical), a VirtualTextureAsset
///        (kind = Virtual, software VT), or a SparseTextureAsset (kind = Sparse,
///        hardware sparse / reserved resource). Only the Physical path is implemented
///        in v1; the Virtual and Sparse arms log and fail until their backends land.

#include "Asset/Loaders/TextureLoader.h"

#include "Asset/Cooked/CookedReader.h"
#include "Asset/Types/PhysicalTextureAsset.h"
#include "Asset/Types/SparseTextureAsset.h"
#include "Asset/Types/VirtualTextureAsset.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

#include <vector>

namespace Hylux::Asset
{
namespace
{

Future<Ref<TextureAsset>> LoadPhysical(const Cooked::CookedReader&   reader,
                                       const AssetLoaderContext&     ctx,
                                       const Cooked::TexturePayload& payload)
{
    RHI::TextureDesc desc{};
    desc.dimension     = static_cast<RHI::TextureDimension>(payload.dimension);
    desc.format        = static_cast<RHI::Format>(payload.format);
    desc.extent.width  = payload.width;
    desc.extent.height = payload.height;
    desc.extent.depth  = 1;
    desc.mipLevels     = payload.mipCount;
    desc.arrayLayers   = payload.arrayLayers == 0 ? 1u : payload.arrayLayers;
    desc.sampleCount   = 1;
    desc.usage         = RHI::TextureUsage::SampledImage | RHI::TextureUsage::TransferDst;
    desc.memory        = RHI::MemoryUsage::GpuOnly;

    Ref<RHI::IRHITexture> texture = ctx.device->CreateTexture(desc);
    if (!texture)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: CreateTexture failed");
        return Future<Ref<TextureAsset>>::MakeFailed();
    }

    PhysicalTextureAsset::InitData init{};
    init.texture                       = texture;
    init.samplerHint.minFilter         = static_cast<RHI::FilterMode>(payload.samplerHintFilter);
    init.samplerHint.magFilter         = init.samplerHint.minFilter;
    init.samplerHint.addressU          = static_cast<RHI::AddressMode>(payload.samplerHintAddress);
    init.samplerHint.addressV          = init.samplerHint.addressU;
    init.samplerHint.addressW          = init.samplerHint.addressU;
    init.samplerHint.maxAnisotropy     = payload.samplerHintMaxAniso;
    const auto bindless                = texture->GetBindlessIndex(RHI::BindlessKind::SrvCbvUav);
    init.bindlessIndex                 = bindless == RHI::BindlessIndex::Invalid
                                             ? 0xFFFFFFFFu
                                             : static_cast<std::uint32_t>(bindless);

    auto mipTable = reader.Range<Cooked::MipEntry>(payload.mipTableOffset, payload.mipCount);
    std::uint64_t totalPixelBytes = 0;
    for (const auto& m : mipTable)
    {
        totalPixelBytes += m.size;
    }
    init.pixelBytes = totalPixelBytes;

    AssetId id{reader.GuidValue(), {}};

    if (ctx.uploader == nullptr || totalPixelBytes == 0)
    {
        Ref<TextureAsset> asset = MakeRef<PhysicalTextureAsset>(std::move(id), std::move(init));
        return Future<Ref<TextureAsset>>::MakeReady(std::move(asset));
    }

    std::vector<MipUpload> uploads;
    uploads.reserve(mipTable.size());
    std::uint32_t levelWidth  = payload.width;
    std::uint32_t levelHeight = payload.height;
    for (std::uint32_t level = 0; level < payload.mipCount; ++level)
    {
        const auto& m = mipTable[level];
        MipUpload   u{};
        u.mipLevel   = level;
        u.arrayLayer = 0;
        u.width      = levelWidth;
        u.height     = levelHeight;
        u.bytes      = reader.ByteRange(payload.pixelDataOffset + m.offset, m.size);
        uploads.push_back(u);
        levelWidth  = levelWidth > 1 ? levelWidth / 2u : 1u;
        levelHeight = levelHeight > 1 ? levelHeight / 2u : 1u;
    }

    return ctx.uploader->UploadTextureMips(*texture, uploads).Transform<Ref<TextureAsset>>(
        [init = std::move(init), id = std::move(id)](bool& ok) mutable -> Ref<TextureAsset> {
            if (!ok)
            {
                HYLUX_LOG_ERROR(LogAsset, "TextureLoader: mip upload failed");
                return {};
            }
            return MakeRef<PhysicalTextureAsset>(std::move(id), std::move(init));
        });
}

} // namespace

Future<Ref<TextureAsset>> TextureLoader::Load(const Cooked::CookedReader& reader, const AssetLoaderContext& ctx)
{
    if (reader.TypeTag() != AssetTypeId::Texture)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: wrong type tag {}", static_cast<int>(reader.TypeTag()));
        return Future<Ref<TextureAsset>>::MakeFailed();
    }
    if (ctx.device == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: missing RHI device");
        return Future<Ref<TextureAsset>>::MakeFailed();
    }

    const auto* payload = reader.PayloadAs<Cooked::TexturePayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: payload missing");
        return Future<Ref<TextureAsset>>::MakeFailed();
    }

    const auto kind = static_cast<TextureAssetKind>(payload->kind);
    switch (kind)
    {
        case TextureAssetKind::Physical:
            return LoadPhysical(reader, ctx, *payload);
        case TextureAssetKind::Virtual:
            HYLUX_LOG_ERROR(LogAsset, "TextureLoader: VirtualTextureAsset (software VT) not yet implemented");
            return Future<Ref<TextureAsset>>::MakeFailed();
        case TextureAssetKind::Sparse:
            HYLUX_LOG_ERROR(LogAsset, "TextureLoader: SparseTextureAsset (hardware sparse) not yet implemented");
            return Future<Ref<TextureAsset>>::MakeFailed();
    }
    HYLUX_LOG_ERROR(LogAsset, "TextureLoader: unknown texture kind {}", static_cast<int>(payload->kind));
    return Future<Ref<TextureAsset>>::MakeFailed();
}

} // namespace Hylux::Asset
