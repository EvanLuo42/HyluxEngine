/// @file
/// @brief TextureAsset — abstract runtime base for every texture-shaped asset. Concrete
///        subclasses are PhysicalTextureAsset (fully-resident, mip chain uploaded whole),
///        VirtualTextureAsset (software-VT: indirection texture + tile-pool atlas +
///        feedback buffer, page residency managed by the engine), and SparseTextureAsset
///        (hardware sparse / reserved resource: one sparse-bound IRHITexture whose tile
///        residency is managed by the driver). Material binding uses only the base API
///        (GetBindlessIndex / GetSamplerHint) so a single material slot can hold any
///        kind without caring which one it is. The kTypeId stays Texture so
///        AssetSubsystem::Load<TextureAsset> dispatches to whichever concrete subclass
///        the loader produced from the cooked payload's kind byte.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetTypeId.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

/// @brief Concrete-subclass discriminator. Mirrors the `kind` byte in
///        Cooked::TexturePayload; values are part of the on-disk format so do not
///        renumber.
enum class TextureAssetKind : std::uint8_t
{
    Physical = 0,
    Virtual  = 1,
    Sparse   = 2,
};

/// @brief Common runtime texture interface. Holds no GPU resources itself; subclasses
///        own whatever combination of textures, page tables, and feedback buffers their
///        kind requires. The bindless index exposed here is the one materials should
///        sample through: for PhysicalTextureAsset it is the texture's SRV slot, for
///        VirtualTextureAsset it is the indirection-table SRV slot (the engine-side
///        feedback writeback is wired through a separate system binding), for
///        SparseTextureAsset it is the sparse-bound texture's SRV slot (the driver
///        handles indirection transparently).
class TextureAsset : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::Texture;

    explicit TextureAsset(AssetId id) noexcept : AssetBase(std::move(id)) {}

    [[nodiscard]] AssetTypeId GetAssetTypeId() const noexcept override { return AssetTypeId::Texture; }

    [[nodiscard]] virtual TextureAssetKind     GetKind() const noexcept           = 0;
    [[nodiscard]] virtual std::uint32_t        GetBindlessIndex() const noexcept  = 0;
    [[nodiscard]] virtual const RHI::SamplerDesc& GetSamplerHint() const noexcept = 0;

protected:
    ~TextureAsset() override = default;
};

} // namespace Hylux::Asset
