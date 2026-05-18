/// @file
/// @brief PhysicalTextureAsset — fully-resident texture. Owns one IRHITexture with a
///        complete mip chain uploaded at load time and a single bindless SRV slot
///        materials sample through. Constructed by TextureLoader when the cooked payload
///        reports TextureAssetKind::Physical.

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

class PhysicalTextureAsset final : public TextureAsset
{
public:
    struct InitData
    {
        Ref<RHI::IRHITexture> texture;
        std::uint32_t         bindlessIndex{0xFFFFFFFFu};
        std::uint64_t         pixelBytes{0};
        RHI::SamplerDesc      samplerHint{};
    };

    PhysicalTextureAsset(AssetId id, InitData data) noexcept
        : TextureAsset(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] TextureAssetKind        GetKind() const noexcept override { return TextureAssetKind::Physical; }
    [[nodiscard]] std::uint32_t           GetBindlessIndex() const noexcept override { return data_.bindlessIndex; }
    [[nodiscard]] const RHI::SamplerDesc& GetSamplerHint() const noexcept override { return data_.samplerHint; }
    [[nodiscard]] std::uint64_t           GetMemoryFootprint() const noexcept override { return data_.pixelBytes; }

    [[nodiscard]] RHI::IRHITexture* GetTexture() const noexcept { return data_.texture.Get(); }

private:
    ~PhysicalTextureAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
