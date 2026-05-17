/// @file
/// @brief Runtime TextureAsset. Owns the RHI texture, the bindless slot index registered
///        against the device's bindless heap, and a sampler hint that materials can use
///        when they need a sampler tied to this texture's intended sampling behaviour.

#pragma once

#include "Asset/AssetBase.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

class TextureAsset final : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::Texture;

    struct InitData
    {
        Ref<RHI::IRHITexture> texture;
        std::uint32_t         bindlessIndex{0xFFFFFFFFu};
        std::uint64_t         pixelBytes{0};
        RHI::SamplerDesc      samplerHint{};
    };

    TextureAsset(AssetId id, InitData data) noexcept
        : AssetBase(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] AssetTypeId  GetAssetTypeId() const noexcept override { return AssetTypeId::Texture; }
    [[nodiscard]] std::uint64_t GetMemoryFootprint() const noexcept override { return data_.pixelBytes; }

    [[nodiscard]] RHI::IRHITexture*       GetTexture() const noexcept { return data_.texture.Get(); }
    [[nodiscard]] std::uint32_t           GetBindlessIndex() const noexcept { return data_.bindlessIndex; }
    [[nodiscard]] const RHI::SamplerDesc& GetSamplerHint() const noexcept { return data_.samplerHint; }

private:
    ~TextureAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
