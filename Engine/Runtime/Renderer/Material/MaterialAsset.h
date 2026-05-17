/// @file
/// @brief Read-only descriptor of a material asset. Holds the asset hash, parameter
///        schema, and the base permutation key shared by every instance. The asset system
///        will eventually own these; stage 6 lets host code construct them inline.

#pragma once

#include "Renderer/Material/MaterialParameter.h"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Read-only material descriptor. Stage 6 ships a hand-built constructor; once an
///        asset pipeline lands the constructor is fed from the cooked asset blob.
class MaterialAsset
{
public:
    struct Config
    {
        std::uint64_t              assetHash{0};
        std::uint64_t              permutationBaseKey{0};
        std::uint32_t              uniformBlockSize{0};
        std::uint32_t              textureCount{0};
        std::vector<ParameterDesc> parameters{};
    };

    MaterialAsset() = default;
    explicit MaterialAsset(Config config) noexcept : config_(std::move(config)) {}

    [[nodiscard]] std::uint64_t                  GetAssetHash() const noexcept { return config_.assetHash; }
    [[nodiscard]] std::uint64_t                  GetPermutationBaseKey() const noexcept { return config_.permutationBaseKey; }
    [[nodiscard]] std::uint32_t                  GetUniformBlockSize() const noexcept { return config_.uniformBlockSize; }
    [[nodiscard]] std::uint32_t                  GetTextureCount() const noexcept { return config_.textureCount; }
    [[nodiscard]] std::span<const ParameterDesc> GetParameters() const noexcept { return config_.parameters; }

    /// @brief Looks up a parameter descriptor by name. Returns nullptr if absent. Linear
    ///        scan — fine for the parameter counts material assets typically carry.
    [[nodiscard]] const ParameterDesc* FindParameter(NameHash name) const noexcept;

private:
    Config config_{};
};

} // namespace Hylux::Renderer
