/// @file
/// @brief Runtime MaterialAsset. Holds the parameter schema, uniform-block layout
///        metadata, shader-map binding, and the default texture references that material
///        instances can override. Constructed by MaterialLoader; held alive by the
///        AssetSubsystem's LRU plus any external consumer Ref<>.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetRef.h"
#include "Asset/Types/MaterialParameter.h"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace Hylux::Shader
{
class MaterialShaderMap;
}

namespace Hylux::Asset
{

class TextureAsset;

/// @brief Read-only material descriptor. The parameter schema, default values, and shader
///        map are baked at cook time; only material instances override individual values.
class MaterialAsset final : public AssetBase
{
public:
    static constexpr AssetTypeId kTypeId = AssetTypeId::Material;

    struct InitData
    {
        std::uint64_t              shaderMapHash{0};
        std::uint64_t              permutationBaseKey{0};
        std::uint32_t              uniformBlockSize{0};
        std::uint32_t              textureCount{0};
        std::vector<ParameterDesc> parameters{};
        std::vector<SoftAssetRef>  defaultTextures{};
        Shader::MaterialShaderMap* shaderMap{nullptr};
    };

    MaterialAsset(AssetId id, InitData data) noexcept
        : AssetBase(std::move(id)), data_(std::move(data))
    {}

    [[nodiscard]] AssetTypeId  GetAssetTypeId() const noexcept override { return AssetTypeId::Material; }
    [[nodiscard]] std::uint64_t GetMemoryFootprint() const noexcept override { return 0; }

    [[nodiscard]] std::uint64_t                  GetShaderMapHash() const noexcept { return data_.shaderMapHash; }
    [[nodiscard]] std::uint64_t                  GetPermutationBaseKey() const noexcept { return data_.permutationBaseKey; }
    [[nodiscard]] std::uint32_t                  GetUniformBlockSize() const noexcept { return data_.uniformBlockSize; }
    [[nodiscard]] std::uint32_t                  GetTextureCount() const noexcept { return data_.textureCount; }
    [[nodiscard]] std::span<const ParameterDesc> GetParameters() const noexcept { return data_.parameters; }
    [[nodiscard]] std::span<const SoftAssetRef>  GetDefaultTextures() const noexcept { return data_.defaultTextures; }
    [[nodiscard]] Shader::MaterialShaderMap*     GetShaderMap() const noexcept { return data_.shaderMap; }

    /// @brief Linear scan for a named parameter descriptor. Returns nullptr if absent.
    [[nodiscard]] const ParameterDesc* FindParameter(NameHash name) const noexcept;

private:
    ~MaterialAsset() override = default;

    InitData data_;
};

} // namespace Hylux::Asset
