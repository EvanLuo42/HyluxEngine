/// @file
/// @brief MeshAsset — abstract runtime base for every mesh-shaped asset. Concrete
///        subclasses are PhysicalMeshAsset (fully-resident VB/IB + DrawIndexed) and
///        VirtualMeshAsset (cluster-page + GPU cull + mesh-shader emit; a.k.a.
///        virtualized geometry / cluster-based mesh streaming). Both paths are supported
///        simultaneously; the renderer dispatches per-primitive based on the concrete
///        kind. The kTypeId stays Mesh so AssetSubsystem::Load<MeshAsset> returns the
///        base; consumers that need the concrete API GetKind()-check and downcast.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetTypeId.h"
#include "Renderer/Proxy/PrimitiveProxy.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

/// @brief Concrete-subclass discriminator. Mirrors the `kind` byte in Cooked::MeshPayload;
///        values are part of the on-disk format so do not renumber.
enum class MeshAssetKind : std::uint8_t
{
    Physical = 0,
    Virtual  = 1,
};

/// @brief Common runtime mesh interface. Holds no GPU resources itself; subclasses own
///        whatever buffer set their kind requires. LocalBounds is shared because every
///        mesh — regardless of representation — feeds the same CPU-side broad-phase cull.
class MeshAsset : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::Mesh;

    MeshAsset(AssetId id, Hylux::Renderer::PrimitiveBounds localBounds) noexcept
        : AssetBase(std::move(id)), localBounds_(localBounds)
    {}

    [[nodiscard]] AssetTypeId GetAssetTypeId() const noexcept override { return AssetTypeId::Mesh; }

    [[nodiscard]] virtual MeshAssetKind                          GetKind() const noexcept = 0;
    [[nodiscard]] const Hylux::Renderer::PrimitiveBounds&        LocalBounds() const noexcept { return localBounds_; }

protected:
    ~MeshAsset() override = default;

private:
    Hylux::Renderer::PrimitiveBounds localBounds_{};
};

} // namespace Hylux::Asset
