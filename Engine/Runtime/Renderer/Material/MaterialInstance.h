/// @file
/// @brief Game-thread material parameter container. Tracks scalar / vector / texture
///        overrides on top of a MaterialAsset and exports them as a MaterialSnapshot for
///        the render thread. Distinct from Asset::MaterialInstanceAsset (the saved-on-disk
///        variant); MaterialInstance is the per-entity *mutable* runtime override holder.

#pragma once

#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/MaterialParameter.h"
#include "Asset/Types/NameHash.h"
#include "Asset/Types/TextureAsset.h"
#include "Core/Memory/Ref.h"
#include "Renderer/Material/MaterialSnapshot.h"

#include <cstdint>
#include <unordered_map>

namespace Hylux::Renderer
{

/// @brief Mutable per-instance material state. Lifetime: a game-side gameplay object (an
///        entity component) typically owns one. Snapshot() produces the immutable payload
///        used to drive render-thread material proxies.
class MaterialInstance
{
public:
    explicit MaterialInstance(const Asset::MaterialAsset* asset) noexcept;

    MaterialInstance(const MaterialInstance&)                = default;
    MaterialInstance& operator=(const MaterialInstance&)     = default;
    MaterialInstance(MaterialInstance&&) noexcept            = default;
    MaterialInstance& operator=(MaterialInstance&&) noexcept = default;

    [[nodiscard]] const Asset::MaterialAsset* GetAsset() const noexcept { return asset_; }

    /// @brief Returns the hash used to key the render-side MaterialProxyCache and to drive
    ///        ShaderSubsystem::GetOrLoadMaterialShaderMap. We expose the asset's shader-map
    ///        hash for both: keeping the proxy-cache key aligned with the shader-map key
    ///        means two materials that share a shader map but differ in instance params
    ///        still hash distinctly via instanceHash.
    [[nodiscard]] std::uint64_t GetAssetHash() const noexcept;

    /// @brief Stores a scalar override. No-op if the parameter is unknown or of a different kind.
    void SetScalar(Asset::NameHash name, float value);

    /// @brief Stores a vector override. Components are stored in vec[0..3].
    void SetVector(Asset::NameHash name, float x, float y, float z, float w);

    /// @brief Stores a texture override. The bindless index is captured from `texture`
    ///        at call time; the instance also retains a strong Ref so the texture stays
    ///        loaded until the override is cleared (Pass null to clear).
    void SetTexture(Asset::NameHash name, Ref<Asset::TextureAsset> texture);

    /// @brief Returns the hash of the current parameter set, recomputed lazily.
    [[nodiscard]] std::uint64_t GetInstanceHash() const noexcept;

    /// @brief Computes the permutation key by mixing the asset's base key with the values
    ///        of every parameter flagged as a permutation contributor.
    [[nodiscard]] std::uint64_t GetPermutationKey() const noexcept;

    /// @brief Materializes a cross-thread snapshot. Cheap (one pass over the parameter
    ///        schema) — the renderer may snapshot once per AssignMaterial call.
    [[nodiscard]] MaterialSnapshot Snapshot() const;

private:
    void MarkDirty() const noexcept;

    const Asset::MaterialAsset*                                                 asset_{nullptr};
    std::unordered_map<Asset::NameHash, Asset::ParameterValue>                  overrides_{};
    std::unordered_map<Asset::NameHash, Ref<Asset::TextureAsset>>               textureRefs_{};

    mutable std::uint64_t cachedInstanceHash_{0};
    mutable std::uint64_t cachedPermutationKey_{0};
    mutable bool          hashDirty_{true};
};

} // namespace Hylux::Renderer
