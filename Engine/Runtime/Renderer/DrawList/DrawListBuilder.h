/// @file
/// @brief Setup-time builder for a DrawList. Path code instantiates one through
///        RenderContext::CreateDrawList, optionally chains filter / upload hooks, then
///        calls Build() to materialize the surviving primitive list (and, in later stages,
///        the instance buffer + indirect args).

#pragma once

#include "Core/Containers/SmallVector.h"
#include "Renderer/DrawList/DrawList.h"
#include "Renderer/DrawList/DrawListDesc.h"
#include "Renderer/Proxy/PrimitiveProxy.h"

#include <functional>
#include <memory>

namespace Hylux
{
class FrameAllocator;
}

namespace Hylux::Renderer
{

class ProxyRegistry;
class SceneView;
class TransformDoubleBuffer;
class UploadHeapManager;

/// @brief Per-primitive predicate evaluated during collection. Returning false drops the
///        primitive from the resulting DrawList.
using DrawFilterFn = std::function<bool(const PrimitiveProxy&)>;

/// @brief Per-primitive instance-data writer evaluated during collection. The writer
///        receives a CPU pointer into the instance buffer's frame slot and stamps in
///        whatever per-instance values the shader expects.
using InstanceUploadFn = std::function<void(const PrimitiveProxy&, void* /*cpuDst*/)>;

/// @brief Builder handle. Owned by the DrawList storage created by RenderContext; the
///        path interacts with it through chained mutators followed by Build().
class DrawListBuilder
{
public:
    DrawListBuilder(const ProxyRegistry*   proxies,
                    TransformDoubleBuffer* transformBuffer,
                    const SceneView*       view,
                    UploadHeapManager*     uploadHeap,
                    FrameAllocator*        frameArena,
                    DrawListDesc           desc) noexcept;

    DrawListBuilder(const DrawListBuilder&)            = delete;
    DrawListBuilder& operator=(const DrawListBuilder&) = delete;
    DrawListBuilder(DrawListBuilder&&) noexcept        = default;
    DrawListBuilder& operator=(DrawListBuilder&&)      = delete;

    /// @brief Attaches a per-primitive predicate. Predicates compose via logical AND.
    DrawListBuilder& WithCustomFilter(DrawFilterFn filter);

    /// @brief Attaches a per-primitive instance-data writer. Invoked once per surviving
    ///        primitive in stage 4b+ when UploadHeapManager allocates the instance buffer.
    DrawListBuilder& WithInstanceUpload(InstanceUploadFn writer);

    /// @brief Materializes the DrawList. Stage 4b CPU collection: applies layerMask,
    ///        optional frustum cull (using SceneView's cached frustum and the proxy's
    ///        world-space AABB), and the optional custom filter. Caps survivors at
    ///        desc.maxDrawCount when non-zero. Instance buffer / indirect args generation
    ///        land with UploadHeapManager.
    [[nodiscard]] std::unique_ptr<DrawList> Build();

private:
    const ProxyRegistry*           proxies_{nullptr};
    TransformDoubleBuffer*         transformBuffer_{nullptr};
    const SceneView*               view_{nullptr};
    UploadHeapManager*             uploadHeap_{nullptr};
    FrameAllocator*                frameArena_{nullptr};
    DrawListDesc                   desc_{};
    SmallVector<DrawFilterFn, 4>   filters_;
    InstanceUploadFn               uploadFn_;
};

} // namespace Hylux::Renderer
