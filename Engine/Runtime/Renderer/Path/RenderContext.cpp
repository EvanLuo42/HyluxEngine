/// @file
/// @brief RenderContext implementation.

#include "Renderer/Path/RenderContext.h"

#include "Renderer/DrawList/DrawListBuilder.h"
#include "Renderer/Proxy/ProxyRegistry.h"

namespace Hylux::Renderer
{

RenderContext::RenderContext(RG::RenderGraph& graph, const SceneView& view, const ProxyRegistry& proxies,
                             RenderResources& resources, std::uint32_t transformBufferBindlessIndex,
                             std::uint64_t renderFrameId, PsoCache* psoCache, TransformDoubleBuffer* transformBuffer,
                             UploadHeapManager* uploadHeap) noexcept
    : graph_(graph), view_(view), proxies_(proxies), resources_(resources),
      transformBufferBindlessIndex_(transformBufferBindlessIndex), renderFrameId_(renderFrameId), psoCache_(psoCache),
      transformBuffer_(transformBuffer), uploadHeap_(uploadHeap)
{}

DrawListBuilder RenderContext::CreateDrawList(const DrawListDesc& desc) const
{
    return DrawListBuilder(&proxies_, transformBuffer_, &view_, uploadHeap_, desc);
}

} // namespace Hylux::Renderer
