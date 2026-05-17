/// @file
/// @brief Runtime MeshAsset. Owns GPU vertex + index buffers, the CPU-side vertex layout
///        + submesh table, and a local-space AABB. Constructed by MeshLoader from a
///        cooked .hass payload; held alive by the AssetSubsystem's LRU plus any external
///        consumer Ref<>.

#pragma once

#include "Asset/AssetBase.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIFormat.h"
#include "Renderer/Proxy/PrimitiveProxy.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Hylux::Asset
{

/// @brief Vertex semantic enum used by the runtime vertex-layout descriptor. Encoded as
///        u16 on disk (Cooked::VertexAttribute::semantic) and translated back to this
///        enum during load.
enum class VertexSemantic : std::uint16_t
{
    Unknown   = 0,
    Position  = 1,
    Normal    = 2,
    Tangent   = 3,
    Color     = 4,
    TexCoord0 = 5,
    TexCoord1 = 6,
    TexCoord2 = 7,
    TexCoord3 = 8,
};

struct VertexAttribute
{
    VertexSemantic semantic{VertexSemantic::Unknown};
    RHI::Format    format{RHI::Format::Unknown};
    std::uint32_t  offset{0};
};

struct Submesh
{
    std::uint32_t indexStart{0};
    std::uint32_t indexCount{0};
    std::uint32_t materialSlot{0};
};

class MeshAsset final : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::Mesh;

    struct InitData
    {
        Ref<RHI::IRHIBuffer>           vertexBuffer;
        Ref<RHI::IRHIBuffer>           indexBuffer;
        RHI::IndexType                 indexType{RHI::IndexType::Uint32};
        std::uint32_t                  vertexCount{0};
        std::uint32_t                  vertexStride{0};
        std::uint32_t                  indexCount{0};
        std::vector<VertexAttribute>   layout{};
        std::vector<Submesh>           submeshes{};
        Hylux::Renderer::PrimitiveBounds localBounds{};
    };

    MeshAsset(AssetId id, InitData data) noexcept
        : AssetBase(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] AssetTypeId GetAssetTypeId() const noexcept override { return AssetTypeId::Mesh; }

    [[nodiscard]] std::uint64_t GetMemoryFootprint() const noexcept override
    {
        const std::uint64_t indexBytes =
            data_.indexCount * static_cast<std::uint64_t>(data_.indexType == RHI::IndexType::Uint16 ? 2u : 4u);
        return static_cast<std::uint64_t>(data_.vertexCount) * data_.vertexStride + indexBytes;
    }

    [[nodiscard]] RHI::IRHIBuffer*                        VertexBuffer() const noexcept { return data_.vertexBuffer.Get(); }
    [[nodiscard]] RHI::IRHIBuffer*                        IndexBuffer() const noexcept { return data_.indexBuffer.Get(); }
    [[nodiscard]] RHI::IndexType                          IndexFormat() const noexcept { return data_.indexType; }
    [[nodiscard]] std::uint32_t                           VertexCount() const noexcept { return data_.vertexCount; }
    [[nodiscard]] std::uint32_t                           VertexStride() const noexcept { return data_.vertexStride; }
    [[nodiscard]] std::uint32_t                           IndexCount() const noexcept { return data_.indexCount; }
    [[nodiscard]] const std::vector<VertexAttribute>&     Layout() const noexcept { return data_.layout; }
    [[nodiscard]] const std::vector<Submesh>&             Submeshes() const noexcept { return data_.submeshes; }
    [[nodiscard]] const Hylux::Renderer::PrimitiveBounds& LocalBounds() const noexcept { return data_.localBounds; }

private:
    ~MeshAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
