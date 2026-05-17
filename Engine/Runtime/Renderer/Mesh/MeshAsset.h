/// @file
/// @brief Minimal mesh asset descriptor. Carries the engine-stable mesh handle plus the
///        local-space AABB the renderer uses to seed PrimitiveProxy bounds at spawn time.
///        Concrete geometry buffers (vertex / index streams, vertex layout) land when the
///        asset system grows real mesh resources.

#pragma once

#include "Renderer/Proxy/PrimitiveProxy.h"

#include <cstdint>
#include <utility>

namespace Hylux::Renderer
{

/// @brief Read-only mesh handle + bounds. Lifetime is owned by the host (asset system or
///        hand-built test data); the renderer copies the relevant scalars into the
///        StructuralCommandQueue payload at spawn time.
class MeshAsset
{
public:
    struct Config
    {
        std::uint64_t   handle{0};
        PrimitiveBounds localBounds{};
    };

    MeshAsset() = default;
    explicit MeshAsset(Config config) noexcept : config_(std::move(config)) {}

    [[nodiscard]] std::uint64_t          GetHandle() const noexcept { return config_.handle; }
    [[nodiscard]] const PrimitiveBounds& GetLocalBounds() const noexcept { return config_.localBounds; }

private:
    Config config_{};
};

} // namespace Hylux::Renderer
