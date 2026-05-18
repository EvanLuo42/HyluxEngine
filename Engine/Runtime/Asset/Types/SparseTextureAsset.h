/// @file
/// @brief SparseTextureAsset — hardware sparse / reserved-resource texture. Stub: owns
///        one IRHITexture created with TextureDesc::sparse = true; the driver exposes the
///        texture as a normal sampleable resource while tile residency is managed by the
///        engine through IAssetUploader::UploadTextureTiles. Material binding looks the
///        same as a PhysicalTextureAsset — one bindless SRV — because the indirection
///        happens entirely in hardware. TextureLoader currently refuses cooked payloads
///        with TextureAssetKind::Sparse; flip the loader switch and the RHI sparse-bind
///        path once both land.

#pragma once

#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

class SparseTextureAsset final : public TextureAsset
{
public:
    struct InitData
    {
        Ref<RHI::IRHITexture> texture;
        std::uint32_t         bindlessIndex{0xFFFFFFFFu};
        std::uint64_t         residentBytes{0};
        RHI::SamplerDesc      samplerHint{};
    };

    SparseTextureAsset(AssetId id, InitData data) noexcept
        : TextureAsset(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] TextureAssetKind        GetKind() const noexcept override { return TextureAssetKind::Sparse; }
    [[nodiscard]] std::uint32_t           GetBindlessIndex() const noexcept override { return data_.bindlessIndex; }
    [[nodiscard]] const RHI::SamplerDesc& GetSamplerHint() const noexcept override { return data_.samplerHint; }
    [[nodiscard]] std::uint64_t           GetMemoryFootprint() const noexcept override { return data_.residentBytes; }

    [[nodiscard]] RHI::IRHITexture* GetTexture() const noexcept { return data_.texture.Get(); }

private:
    ~SparseTextureAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
