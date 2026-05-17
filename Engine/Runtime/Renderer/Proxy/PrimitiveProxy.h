/// @file
/// @brief Render-thread primitive proxy record. One entry per live drawable; lives in
///        ProxyRegistry. Transform is stored separately in TransformDoubleBuffer and
///        addressed through the proxy's id (which equals its transform slot index).

#pragma once

#include "Renderer/Proxy/ProxyId.h"
#include "Renderer/RendererForward.h"

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Local-space axis-aligned bounding box. Stage 4a placeholder; real bounds are
///        provided when the asset pipeline lands a Mesh asset.
struct PrimitiveBounds
{
    float minX{0.0f}, minY{0.0f}, minZ{0.0f};
    float maxX{0.0f}, maxY{0.0f}, maxZ{0.0f};
};

/// @brief Compact render-side state for one primitive. Stable across frames; mutated only
///        by the render thread in response to StructuralCommands.
struct PrimitiveProxy
{
    ProxyId         id{ProxyId::Invalid};
    std::uint32_t   transformIndex{0};      // matches the slot in TransformDoubleBuffer
    std::uint64_t   meshHandle{0};          // opaque until the mesh asset system lands
    MaterialProxy*  materialProxy{nullptr}; // resolved by MaterialProxyCache; non-owning
    std::uint32_t   flags{0};               // visibility / per-primitive bits
    std::uint32_t   layerMask{0xFFFFFFFFu};
    PrimitiveBounds bounds{};
};

} // namespace Hylux::Renderer
