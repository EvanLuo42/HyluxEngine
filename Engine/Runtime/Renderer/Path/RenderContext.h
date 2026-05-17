/// @file
/// @brief Author-facing handle passed to IRenderPath::BuildGraph. Provides access to the
///        per-frame RenderGraph, the current view, and the DrawListBuilder. Hides anything
///        below the renderer boundary.

#pragma once

#include "Renderer/RendererForward.h"

#include <cstdint>

namespace Hylux::RG
{
class RenderGraph;
}

namespace Hylux::Renderer
{

class SceneView;
class ProxyRegistry;
class RenderResources;

/// @brief Setup-time context for a path. The renderer constructs one per (frame, view) and
///        hands it to IRenderPath::BuildGraph. Lifetime is one BuildGraph invocation.
class RenderContext
{
public:
    RenderContext(RG::RenderGraph& graph, const SceneView& view, const ProxyRegistry& proxies,
                  RenderResources& resources, std::uint32_t transformBufferBindlessIndex, std::uint64_t renderFrameId,
                  PsoCache* psoCache, TransformDoubleBuffer* transformBuffer, UploadHeapManager* uploadHeap) noexcept;

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    RenderContext(RenderContext&&) = delete;
    RenderContext& operator=(RenderContext&&) = delete;

    [[nodiscard]] RG::RenderGraph&       GetGraph() const noexcept { return graph_; }
    [[nodiscard]] const SceneView&       GetView() const noexcept { return view_; }
    [[nodiscard]] const ProxyRegistry&   GetProxies() const noexcept { return proxies_; }
    [[nodiscard]] RenderResources&       GetResources() const noexcept { return resources_; }
    [[nodiscard]] PsoCache*              GetPsoCache() const noexcept { return psoCache_; }
    [[nodiscard]] TransformDoubleBuffer* GetTransformBuffer() const noexcept { return transformBuffer_; }
    [[nodiscard]] UploadHeapManager*     GetUploadHeap() const noexcept { return uploadHeap_; }

    /// @brief Returns the bindless slot of the currently-readable transform half. A path
    ///        forwards this as a push constant so its shaders can index into the SSBO.
    ///        0xFFFFFFFF when the renderer has not yet published a transform half.
    [[nodiscard]] std::uint32_t GetTransformBufferBindlessIndex() const noexcept
    {
        return transformBufferBindlessIndex_;
    }

    /// @brief Render-thread iteration counter for the frame being built. Paths use it to
    ///        index into host-supplied resource rings (e.g. readback buffers) so that
    ///        consecutive re-renders of the same cached scene write to distinct slots.
    [[nodiscard]] std::uint64_t GetRenderFrameId() const noexcept { return renderFrameId_; }

    /// @brief Convenience: returns a DrawListBuilder that reads from this view's proxy
    ///        registry and writes per-instance data into the current transform buffer half.
    ///        Builder is value-returned; chain `.WithCustomFilter(...).Build()`.
    [[nodiscard]] DrawListBuilder CreateDrawList(const DrawListDesc& desc) const;

private:
    RG::RenderGraph&       graph_;
    const SceneView&       view_;
    const ProxyRegistry&   proxies_;
    RenderResources&       resources_;
    std::uint32_t          transformBufferBindlessIndex_{0xFFFFFFFFu};
    std::uint64_t          renderFrameId_{0};
    PsoCache*              psoCache_{nullptr};
    TransformDoubleBuffer* transformBuffer_{nullptr};
    UploadHeapManager*     uploadHeap_{nullptr};
};

} // namespace Hylux::Renderer
