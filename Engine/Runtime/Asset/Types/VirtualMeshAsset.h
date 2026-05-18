/// @file
/// @brief VirtualMeshAsset — cluster-page-streamed mesh (a.k.a. virtualized geometry).
///        Stub: declares the runtime shape (cluster page buffer, cluster metadata, LOD
///        DAG, page table) the renderer will consume from a GPU cluster-cull pass +
///        mesh-shader emit, but the streaming / residency pipeline behind it is not
///        implemented in v1. MeshLoader currently refuses cooked payloads with
///        MeshAssetKind::Virtual; flip the loader switch once the cluster-page upload +
///        cull-pass scaffolding lands.

#pragma once

#include "Asset/Types/MeshAsset.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Hylux::Asset
{

/// @brief One contiguous cluster range bound to a material slot. The renderer issues one
///        cluster-cull dispatch per slot when the virtual mesh is drawn.
struct ClusterSlot
{
    std::uint32_t firstCluster{0};
    std::uint32_t clusterCount{0};
    std::uint32_t materialSlot{0};
};

class VirtualMeshAsset final : public MeshAsset
{
public:
    struct InitData
    {
        Ref<RHI::IRHIBuffer>             clusterPageBuffer;
        Ref<RHI::IRHIBuffer>             clusterMetaBuffer;
        Ref<RHI::IRHIBuffer>             lodDagBuffer;
        Ref<RHI::IRHIBuffer>             pageTableBuffer;
        std::vector<ClusterSlot>         materialSlots{};
        std::uint32_t                    clusterCount{0};
        std::uint32_t                    rootClusterIndex{0};
        std::uint32_t                    pageCount{0};
        std::uint64_t                    residentBytes{0};
        Hylux::Renderer::PrimitiveBounds localBounds{};
    };

    VirtualMeshAsset(AssetId id, InitData data) noexcept
        : MeshAsset(std::move(id), data.localBounds), data_(std::move(data))
    {}

    [[nodiscard]] MeshAssetKind  GetKind() const noexcept override { return MeshAssetKind::Virtual; }
    [[nodiscard]] std::uint64_t  GetMemoryFootprint() const noexcept override { return data_.residentBytes; }

    [[nodiscard]] RHI::IRHIBuffer*                ClusterPageBuffer() const noexcept { return data_.clusterPageBuffer.Get(); }
    [[nodiscard]] RHI::IRHIBuffer*                ClusterMetaBuffer() const noexcept { return data_.clusterMetaBuffer.Get(); }
    [[nodiscard]] RHI::IRHIBuffer*                LodDagBuffer() const noexcept { return data_.lodDagBuffer.Get(); }
    [[nodiscard]] RHI::IRHIBuffer*                PageTableBuffer() const noexcept { return data_.pageTableBuffer.Get(); }
    [[nodiscard]] const std::vector<ClusterSlot>& MaterialSlots() const noexcept { return data_.materialSlots; }
    [[nodiscard]] std::uint32_t                   ClusterCount() const noexcept { return data_.clusterCount; }
    [[nodiscard]] std::uint32_t                   RootClusterIndex() const noexcept { return data_.rootClusterIndex; }
    [[nodiscard]] std::uint32_t                   PageCount() const noexcept { return data_.pageCount; }

private:
    ~VirtualMeshAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
