/// @file
/// @brief TextureLoader implementation. Creates a 2D texture with the declared mip
///        chain, stages per-mip pixel bytes via ctx.uploader, and registers the
///        resulting bindless slot on the runtime asset.

#include "Asset/Loaders/TextureLoader.h"

#include "Asset/Cooked/CookedReader.h"
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

Ref<TextureAsset> TextureLoader::Load(const Cooked::CookedReader& reader, const AssetLoaderContext& ctx)
{
    if (reader.TypeTag() != AssetTypeId::Texture)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: wrong type tag {}", static_cast<int>(reader.TypeTag()));
        return {};
    }
    if (ctx.device == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: missing RHI device");
        return {};
    }

    const auto* payload = reader.PayloadAs<Cooked::TexturePayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: payload missing");
        return {};
    }

    RHI::TextureDesc desc{};
    desc.dimension     = static_cast<RHI::TextureDimension>(payload->dimension);
    desc.format        = static_cast<RHI::Format>(payload->format);
    desc.extent.width  = payload->width;
    desc.extent.height = payload->height;
    desc.extent.depth  = 1;
    desc.mipLevels     = payload->mipCount;
    desc.arrayLayers   = payload->arrayLayers == 0 ? 1u : payload->arrayLayers;
    desc.sampleCount   = 1;
    desc.usage         = RHI::TextureUsage::SampledImage | RHI::TextureUsage::TransferDst;
    desc.memory        = RHI::MemoryUsage::GpuOnly;

    Ref<RHI::IRHITexture> texture = ctx.device->CreateTexture(desc);
    if (!texture)
    {
        HYLUX_LOG_ERROR(LogAsset, "TextureLoader: CreateTexture failed");
        return {};
    }

    TextureAsset::InitData init{};
    init.texture                       = texture;
    init.samplerHint.minFilter         = static_cast<RHI::FilterMode>(payload->samplerHintFilter);
    init.samplerHint.magFilter         = init.samplerHint.minFilter;
    init.samplerHint.addressU          = static_cast<RHI::AddressMode>(payload->samplerHintAddress);
    init.samplerHint.addressV          = init.samplerHint.addressU;
    init.samplerHint.addressW          = init.samplerHint.addressU;
    init.samplerHint.maxAnisotropy     = payload->samplerHintMaxAniso;
    const auto bindless                = texture->GetBindlessIndex(RHI::BindlessKind::SrvCbvUav);
    init.bindlessIndex                 = bindless == RHI::BindlessIndex::Invalid
                                             ? 0xFFFFFFFFu
                                             : static_cast<std::uint32_t>(bindless);

    auto mipTable = reader.Range<Cooked::MipEntry>(payload->mipTableOffset, payload->mipCount);
    std::uint64_t totalPixelBytes = 0;
    for (const auto& m : mipTable)
    {
        totalPixelBytes += m.size;
    }
    init.pixelBytes = totalPixelBytes;

    if (ctx.uploader != nullptr && totalPixelBytes > 0)
    {
        std::vector<MipUpload> uploads;
        uploads.reserve(mipTable.size());
        std::uint32_t levelWidth  = payload->width;
        std::uint32_t levelHeight = payload->height;
        for (std::uint32_t level = 0; level < payload->mipCount; ++level)
        {
            const auto& m = mipTable[level];
            MipUpload   u{};
            u.mipLevel   = level;
            u.arrayLayer = 0;
            u.width      = levelWidth;
            u.height     = levelHeight;
            u.bytes      = reader.ByteRange(payload->pixelDataOffset + m.offset, m.size);
            uploads.push_back(u);
            levelWidth  = levelWidth > 1 ? levelWidth / 2u : 1u;
            levelHeight = levelHeight > 1 ? levelHeight / 2u : 1u;
        }
        if (!ctx.uploader->UploadTextureMips(*texture, uploads))
        {
            HYLUX_LOG_ERROR(LogAsset, "TextureLoader: mip upload failed");
            return {};
        }
    }

    AssetId id{reader.GuidValue(), {}};
    return MakeRef<TextureAsset>(std::move(id), std::move(init));
}

} // namespace Hylux::Asset
