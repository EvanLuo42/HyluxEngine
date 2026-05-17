/// @file
/// @brief Declarative description of what DrawListBuilder should collect, how to sort it,
///        and how to draw it. Fully decoupled from any runtime data; the desc is what the
///        path author hands to RenderContext::CreateDrawList.

#pragma once

#include "Renderer/DrawList/InstanceDataLayout.h"

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Drawable bucket. Drives default sort + render-state defaults; custom buckets
///        let paths attach arbitrary user passes.
enum class DrawBucket : std::uint8_t
{
    Opaque,
    AlphaTested,
    Transparent,
    Decal,
    Shadow,
    Velocity,
    Custom,
};

/// @brief Ordering applied by DrawListBuilder to collected drawables.
enum class SortMode : std::uint8_t
{
    None,
    FrontToBack,
    BackToFront,
    MaterialThenDepth,
    SortKey64,   // caller-supplied per-primitive 64-bit key
};

/// @brief Full declarative DrawList description. Mostly POD; held by value inside the path.
struct DrawListDesc
{
    DrawBucket         bucket{DrawBucket::Opaque};
    SortMode           sortMode{SortMode::MaterialThenDepth};
    std::uint64_t      passNameHash{0};
    std::uint32_t      layerMask{0xFFFFFFFFu};
    std::uint32_t      renderQueueLow{0};
    std::uint32_t      renderQueueHigh{0xFFFFFFFFu};
    std::uint32_t      viewMask{0};
    bool               applyFrustumCulling{true};
    bool               applyOcclusionCulling{false};
    bool               useGpuCulling{true};
    bool               useIndirectDraw{true};
    InstanceDataLayout instanceLayout{};
    std::uint32_t      maxDrawCount{0};
    std::uint64_t      userTag{0};
};

} // namespace Hylux::Renderer
