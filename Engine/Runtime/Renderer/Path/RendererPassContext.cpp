#include "Renderer/Path/RendererPassContext.h"

#include "RHI/IRHICommandList.h"
#include "RHI/RHITypes.h"
#include "Renderer/DrawList/DrawList.h"
#include "Renderer/Pso/PipelineRenderState.h"
#include "Renderer/Pso/PsoCache.h"
#include "Renderer/View/SceneView.h"

#include <algorithm>
#include <cstdint>
#include <span>

namespace Hylux::Renderer
{
namespace
{

/// @brief Canonical per-draw push-constant payload. Matches the layout shaders are
///        expected to declare via `[[vk::push_constant]] struct RendererDrawPush`. Stage
///        4b ships the layout — actual shader bindings follow.
struct RendererDrawPush
{
    std::uint32_t transformBindless;
    std::uint32_t instanceBindless;
    std::uint32_t viewMask;
    std::uint32_t reserved;
};

/// @brief Stride of a non-indexed indirect command (VkDrawIndirectCommand = 4 uint32s:
///        vertexCount, instanceCount, firstVertex, firstInstance). Matches the backend's
///        vkCmdDrawIndirectCount mapping; indexed-indirect path would be 5 uint32s.
constexpr std::uint32_t kNonIndexedDrawIndirectStride = sizeof(std::uint32_t) * 4;

} // namespace

RendererPassContext::RendererPassContext(RG::RGContext& rgContext, RHI::IRHICommandList& cmd, const SceneView& view,
                                         PsoCache* psoCache, std::uint32_t transformBindlessIndex,
                                         const PsoFormatKey& formats) noexcept
    : rgContext_(rgContext), cmd_(cmd), view_(view), psoCache_(psoCache),
      transformBindlessIndex_(transformBindlessIndex), formats_(formats)
{}

void RendererPassContext::DrawRendererList(const DrawList& list)
{
    if (psoCache_ == nullptr)
    {
        return;
    }

    static constexpr PipelineRenderState kDefaultState{};

    const DrawListDesc& desc = list.GetDesc();

    PsoRequest req{};
    req.passNameHash = desc.passNameHash;
    req.permutationKey = 0;
    req.materialAssetHash = 0;
    req.renderState = &kDefaultState;
    req.formats = formats_;

    const PsoHandle handle = psoCache_->GetOrCreate(req);
    if (!handle)
    {
        // Resolution failed (e.g. missing shader archive entry). PsoCache already logged;
        // silently skip so we don't spam during editor bring-up.
        return;
    }

    cmd_.SetPipelineLayout(handle.layout);
    cmd_.SetGraphicsPipeline(handle.pipeline);

    const RHI::Rect2D   rect = view_.GetTargetRect();
    const RHI::Viewport viewport{
        .x = static_cast<float>(rect.offset.x),
        .y = static_cast<float>(rect.offset.y),
        .width = static_cast<float>(rect.extent.width),
        .height = static_cast<float>(rect.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    cmd_.SetViewports(std::span{&viewport, 1});
    cmd_.SetScissors(std::span{&rect, 1});

    if (handle.pushConstantSize > 0)
    {
        RendererDrawPush push{};
        push.transformBindless = transformBindlessIndex_;
        push.instanceBindless = list.GetInstanceBindlessIndex();
        push.viewMask = desc.viewMask;

        const std::uint32_t copySize =
            std::min<std::uint32_t>(handle.pushConstantSize, static_cast<std::uint32_t>(sizeof(push)));
        cmd_.SetPushConstants(0, copySize, &push);
    }

    if (list.GetDrawCount() == 0 || list.GetIndirectArgs() == nullptr || list.GetIndirectCount() == nullptr)
    {
        return;
    }

    cmd_.DrawIndirectCount(list.GetIndirectArgs(), list.GetIndirectArgsOffset(), list.GetIndirectCount(),
                           list.GetIndirectCountOffset(), list.GetDrawCount(), kNonIndexedDrawIndirectStride);
}

} // namespace Hylux::Renderer
