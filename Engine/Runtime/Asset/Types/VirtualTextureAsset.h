/// @file
/// @brief VirtualTextureAsset — software virtual texture: a small physical indirection
///        texture + a physical tile-pool atlas + a GPU feedback buffer the engine reads
///        to drive page residency. Materials sample through the indirection slot; the
///        shader resolves the page and fetches from the atlas. Distinct from
///        SparseTextureAsset, which uses hardware sparse / reserved resources and looks
///        like a single normal texture to materials.
///
///        Stub: declares the runtime shape so material binding and the asset registry
///        can already address it, but the page-residency / feedback-drain pipeline
///        behind it is not implemented in v1. TextureLoader currently refuses cooked
///        payloads with TextureAssetKind::Virtual; flip the loader switch once the VT
///        feedback + page-table backend lands.

#pragma once

#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>
#include <utility>

namespace Hylux::Asset
{

class VirtualTextureAsset final : public TextureAsset
{
public:
    struct InitData
    {
        Ref<RHI::IRHITexture> indirectionTexture;
        Ref<RHI::IRHITexture> tilePool;
        Ref<RHI::IRHIBuffer>  feedbackBuffer;
        std::uint32_t         indirectionBindlessIndex{0xFFFFFFFFu};
        std::uint32_t         feedbackBindlessIndex{0xFFFFFFFFu};
        std::uint64_t         residentBytes{0};
        RHI::SamplerDesc      samplerHint{};
    };

    VirtualTextureAsset(AssetId id, InitData data) noexcept
        : TextureAsset(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] TextureAssetKind        GetKind() const noexcept override { return TextureAssetKind::Virtual; }
    [[nodiscard]] std::uint32_t           GetBindlessIndex() const noexcept override { return data_.indirectionBindlessIndex; }
    [[nodiscard]] const RHI::SamplerDesc& GetSamplerHint() const noexcept override { return data_.samplerHint; }
    [[nodiscard]] std::uint64_t           GetMemoryFootprint() const noexcept override { return data_.residentBytes; }

    [[nodiscard]] RHI::IRHITexture* GetIndirectionTexture() const noexcept { return data_.indirectionTexture.Get(); }
    [[nodiscard]] RHI::IRHITexture* GetTilePool() const noexcept { return data_.tilePool.Get(); }
    [[nodiscard]] RHI::IRHIBuffer*  GetFeedbackBuffer() const noexcept { return data_.feedbackBuffer.Get(); }
    [[nodiscard]] std::uint32_t     GetFeedbackBindlessIndex() const noexcept { return data_.feedbackBindlessIndex; }

private:
    ~VirtualTextureAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
