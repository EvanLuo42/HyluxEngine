/// @file
/// @brief MeshLoader implementation. Dispatches on Cooked::MeshPayload::kind to produce
///        either a PhysicalMeshAsset (kind = Physical) or a VirtualMeshAsset (kind =
///        Virtual). The Virtual path is not implemented in v1 — the loader logs and
///        fails until the cluster-page upload + cull-pass scaffolding lands.

#include "Asset/Loaders/MeshLoader.h"

#include "Asset/Cooked/CookedReader.h"
#include "Asset/Types/PhysicalMeshAsset.h"
#include "Asset/Types/VirtualMeshAsset.h"
#include "Core/Async/WhenAll.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/MakeRef.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

#include <vector>

namespace Hylux::Asset
{
namespace
{

Ref<RHI::IRHIBuffer> CreateDeviceBuffer(RHI::IRHIDevice&  device,
                                        std::uint64_t      size,
                                        RHI::BufferUsage   usage)
{
    if (size == 0)
    {
        return {};
    }
    RHI::BufferDesc desc{};
    desc.size   = size;
    desc.usage  = usage | RHI::BufferUsage::TransferDst;
    desc.memory = RHI::MemoryUsage::GpuOnly;
    return device.CreateBuffer(desc);
}

Future<Ref<MeshAsset>> LoadPhysical(const Cooked::CookedReader& reader,
                                    const AssetLoaderContext&   ctx,
                                    const Cooked::MeshPayload&  payload)
{
    PhysicalMeshAsset::InitData init{};
    init.vertexCount  = payload.vertexCount;
    init.vertexStride = payload.vertexStride;
    init.indexCount   = payload.indexCount;
    init.indexType    = payload.indexFormat == 0 ? RHI::IndexType::Uint16 : RHI::IndexType::Uint32;
    init.localBounds.minX = payload.localMin[0];
    init.localBounds.minY = payload.localMin[1];
    init.localBounds.minZ = payload.localMin[2];
    init.localBounds.maxX = payload.localMax[0];
    init.localBounds.maxY = payload.localMax[1];
    init.localBounds.maxZ = payload.localMax[2];

    auto layoutEntries =
        reader.Range<Cooked::VertexAttribute>(payload.vertexLayoutOffset, payload.vertexLayoutCount);
    init.layout.reserve(layoutEntries.size());
    for (const auto& e : layoutEntries)
    {
        VertexAttribute a{};
        a.semantic = static_cast<VertexSemantic>(e.semantic);
        a.format   = static_cast<RHI::Format>(e.format);
        a.offset   = e.offset;
        init.layout.push_back(a);
    }

    auto submeshEntries = reader.Range<Cooked::Submesh>(payload.submeshOffset, payload.submeshCount);
    init.submeshes.reserve(submeshEntries.size());
    for (const auto& e : submeshEntries)
    {
        init.submeshes.push_back({e.indexStart, e.indexCount, e.materialSlot});
    }

    const std::uint64_t vertexBytes = static_cast<std::uint64_t>(init.vertexCount) * init.vertexStride;
    const std::uint64_t indexBytes =
        static_cast<std::uint64_t>(init.indexCount) * (init.indexType == RHI::IndexType::Uint16 ? 2u : 4u);

    init.vertexBuffer = CreateDeviceBuffer(*ctx.device, vertexBytes, RHI::BufferUsage::VertexBuffer);
    init.indexBuffer  = CreateDeviceBuffer(*ctx.device, indexBytes, RHI::BufferUsage::IndexBuffer);
    if ((vertexBytes > 0 && !init.vertexBuffer) || (indexBytes > 0 && !init.indexBuffer))
    {
        HYLUX_LOG_ERROR(LogAsset, "MeshLoader: GPU buffer creation failed (vb={} ib={})",
                        static_cast<bool>(init.vertexBuffer), static_cast<bool>(init.indexBuffer));
        return Future<Ref<MeshAsset>>::MakeFailed();
    }

    AssetId id{reader.GuidValue(), {}};

    std::vector<Future<bool>> uploads;
    if (ctx.uploader != nullptr)
    {
        if (init.vertexBuffer && vertexBytes > 0)
        {
            auto vbBytes = reader.ByteRange(payload.vertexBytesOffset, static_cast<std::uint32_t>(vertexBytes));
            uploads.push_back(ctx.uploader->UploadBufferData(*init.vertexBuffer, vbBytes.data(), vertexBytes));
        }
        if (init.indexBuffer && indexBytes > 0)
        {
            auto ibBytes = reader.ByteRange(payload.indexBytesOffset, static_cast<std::uint32_t>(indexBytes));
            uploads.push_back(ctx.uploader->UploadBufferData(*init.indexBuffer, ibBytes.data(), indexBytes));
        }
    }

    if (uploads.empty())
    {
        Ref<MeshAsset> asset = MakeRef<PhysicalMeshAsset>(std::move(id), std::move(init));
        return Future<Ref<MeshAsset>>::MakeReady(std::move(asset));
    }

    return WhenAll(std::move(uploads)).Transform<Ref<MeshAsset>>(
        [init = std::move(init), id = std::move(id)](std::vector<bool>& results) mutable -> Ref<MeshAsset> {
            for (bool ok : results)
            {
                if (!ok)
                {
                    HYLUX_LOG_ERROR(LogAsset, "MeshLoader: GPU buffer upload failed");
                    return {};
                }
            }
            return MakeRef<PhysicalMeshAsset>(std::move(id), std::move(init));
        });
}

} // namespace

Future<Ref<MeshAsset>> MeshLoader::Load(const Cooked::CookedReader& reader, const AssetLoaderContext& ctx)
{
    if (reader.TypeTag() != AssetTypeId::Mesh)
    {
        HYLUX_LOG_ERROR(LogAsset, "MeshLoader: wrong type tag {}", static_cast<int>(reader.TypeTag()));
        return Future<Ref<MeshAsset>>::MakeFailed();
    }
    if (ctx.device == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "MeshLoader: missing RHI device");
        return Future<Ref<MeshAsset>>::MakeFailed();
    }

    const auto* payload = reader.PayloadAs<Cooked::MeshPayload>();
    if (payload == nullptr)
    {
        HYLUX_LOG_ERROR(LogAsset, "MeshLoader: payload missing");
        return Future<Ref<MeshAsset>>::MakeFailed();
    }

    const auto kind = static_cast<MeshAssetKind>(payload->kind);
    switch (kind)
    {
        case MeshAssetKind::Physical:
            return LoadPhysical(reader, ctx, *payload);
        case MeshAssetKind::Virtual:
            HYLUX_LOG_ERROR(LogAsset, "MeshLoader: VirtualMeshAsset not yet implemented");
            return Future<Ref<MeshAsset>>::MakeFailed();
    }
    HYLUX_LOG_ERROR(LogAsset, "MeshLoader: unknown mesh kind {}", static_cast<int>(payload->kind));
    return Future<Ref<MeshAsset>>::MakeFailed();
}

} // namespace Hylux::Asset
