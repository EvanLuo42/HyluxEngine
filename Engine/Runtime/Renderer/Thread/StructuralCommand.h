/// @file
/// @brief POD command types and their std::variant aggregation. Game thread fills a
///        StructuralCommand and hands it to the StructuralCommandQueue; render thread
///        drains and dispatches. Stage 3 keeps the payloads tiny — material / mesh handles
///        are opaque uint64s until the asset system lands.

#pragma once

#include "Renderer/Material/MaterialSnapshot.h"
#include "Renderer/Proxy/ProxyId.h"
#include "Renderer/View/SceneViewRequest.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Allocates a render-side primitive slot. The id doubles as the TransformDoubleBuffer
///        slot index, so the render thread can store a PrimitiveProxy with the same handle.
struct SpawnPrimitiveCmd
{
    ProxyId       id{ProxyId::Invalid};
    std::uint32_t layerMask{0xFFFFFFFFu};
};

/// @brief Retires a previously spawned primitive. Stage 3: no-op until ProxyRegistry has
///        live storage.
struct DespawnPrimitiveCmd
{
    ProxyId id{ProxyId::Invalid};
};

/// @brief Reassigns a primitive's material. Carries the full snapshot so the render thread
///        can resolve it through MaterialProxyCache without any back-channel to game state.
struct AssignMaterialCmd
{
    ProxyId          id{ProxyId::Invalid};
    MaterialSnapshot snapshot{};
};

/// @brief Reassigns a primitive's mesh asset. Opaque handle until the asset system arrives.
struct AssignMeshCmd
{
    ProxyId       id{ProxyId::Invalid};
    std::uint64_t meshHandle{0};
};

/// @brief Updates layer / visibility / render-queue flags on a primitive.
struct SetFlagsCmd
{
    ProxyId       id{ProxyId::Invalid};
    std::uint32_t flags{0};
};

/// @brief Updates the local-space AABB used by CPU / GPU culling. Coordinates are in the
///        primitive's local space; the world transform from TransformDoubleBuffer composes
///        them at cull time.
struct SetBoundsCmd
{
    ProxyId id{ProxyId::Invalid};
    float   minX{0.0f}, minY{0.0f}, minZ{0.0f};
    float   maxX{0.0f}, maxY{0.0f}, maxZ{0.0f};
};

/// @brief Frame boundary marker. Carries the view list the render thread should consume
///        once it drains the queue up to and including this command.
struct BeginFrameCmd
{
    std::uint64_t                 frameId{0};
    std::vector<SceneViewRequest> views{};
};

/// @brief Tagged union of every command the render thread accepts. Render-thread dispatch
///        does std::visit; game-thread enqueue does emplace or push.
using StructuralCommand = std::variant<SpawnPrimitiveCmd,
                                       DespawnPrimitiveCmd,
                                       AssignMaterialCmd,
                                       AssignMeshCmd,
                                       SetFlagsCmd,
                                       SetBoundsCmd,
                                       BeginFrameCmd>;

} // namespace Hylux::Renderer
