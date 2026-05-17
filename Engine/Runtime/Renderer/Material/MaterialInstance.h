/// @file
/// @brief Game-thread material parameter container. Tracks scalar / vector / texture
///        overrides on top of a MaterialAsset and exports them as a MaterialSnapshot for
///        the render thread.

#pragma once

#include "Renderer/Material/MaterialAsset.h"
#include "Renderer/Material/MaterialParameter.h"
#include "Renderer/Material/MaterialSnapshot.h"
#include "Renderer/Material/NameHash.h"

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
    explicit MaterialInstance(const MaterialAsset* asset) noexcept;

    MaterialInstance(const MaterialInstance&)            = default;
    MaterialInstance& operator=(const MaterialInstance&) = default;
    MaterialInstance(MaterialInstance&&) noexcept        = default;
    MaterialInstance& operator=(MaterialInstance&&) noexcept = default;

    [[nodiscard]] const MaterialAsset* GetAsset() const noexcept { return asset_; }
    [[nodiscard]] std::uint64_t        GetAssetHash() const noexcept;

    /// @brief Stores a scalar override. No-op if the parameter is unknown or of a different kind.
    void SetScalar(NameHash name, float value);

    /// @brief Stores a vector override. Components are stored in vec[0..3].
    void SetVector(NameHash name, float x, float y, float z, float w);

    /// @brief Stores a texture handle override. The handle is opaque to the renderer.
    void SetTexture(NameHash name, std::uint64_t textureHandle);

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

    const MaterialAsset*                          asset_{nullptr};
    std::unordered_map<NameHash, ParameterValue>  overrides_{};

    mutable std::uint64_t                         cachedInstanceHash_{0};
    mutable std::uint64_t                         cachedPermutationKey_{0};
    mutable bool                                  hashDirty_{true};
};

} // namespace Hylux::Renderer
