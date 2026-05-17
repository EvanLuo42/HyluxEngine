/// @file
/// @brief Render-thread owned mirror of the renderable scene. Mutated exclusively through
///        the StructuralCommandQueue; read by IRenderPath / DrawListBuilder.

#pragma once

#include "Renderer/Proxy/PrimitiveProxy.h"
#include "Renderer/Proxy/ProxyId.h"
#include "Renderer/RendererForward.h"

#include <cstdint>
#include <unordered_map>

namespace Hylux::Renderer
{

/// @brief Render-thread scene mirror. Spawn / Despawn / Assign* / SetFlags mutate the map;
///        TransformIndex on each proxy points back into the TransformDoubleBuffer so the
///        DrawListBuilder can correlate per-instance data with shader-visible transforms.
class ProxyRegistry
{
public:
    ProxyRegistry() = default;
    ~ProxyRegistry() = default;

    ProxyRegistry(const ProxyRegistry&) = delete;
    ProxyRegistry& operator=(const ProxyRegistry&) = delete;

    /// @brief Inserts a proxy with the supplied transform slot index. Render-thread only.
    PrimitiveProxy& Spawn(ProxyId id, std::uint32_t transformIndex, std::uint32_t layerMask);

    /// @brief Removes a proxy if present. Render-thread only.
    void Despawn(ProxyId id);

    /// @brief Updates the resolved material proxy pointer of a live proxy. No-op if unknown.
    void AssignMaterial(ProxyId id, MaterialProxy* materialProxy);

    /// @brief Updates the mesh handle of a live proxy. No-op if unknown.
    void AssignMesh(ProxyId id, std::uint64_t meshHandle);

    /// @brief Replaces the flags / layer mask of a live proxy. No-op if unknown.
    void SetFlags(ProxyId id, std::uint32_t flags);

    /// @brief Stores the local-space AABB used by frustum culling. No-op if unknown.
    void SetBounds(ProxyId id, const PrimitiveBounds& bounds);

    /// @brief Returns a non-owning pointer to the proxy, or nullptr when not present.
    [[nodiscard]] const PrimitiveProxy* Find(ProxyId id) const noexcept;

    /// @brief Read-only access to the underlying map. Used by DrawListBuilder to iterate
    ///        live proxies during collection.
    [[nodiscard]] const std::unordered_map<ProxyId, PrimitiveProxy>& Entries() const noexcept { return primitives_; }

    [[nodiscard]] std::uint32_t GetPrimitiveCount() const noexcept
    {
        return static_cast<std::uint32_t>(primitives_.size());
    }

private:
    std::unordered_map<ProxyId, PrimitiveProxy> primitives_;
};

} // namespace Hylux::Renderer
