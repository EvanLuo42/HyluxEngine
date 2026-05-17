/// @file
/// @brief DrawListBuilder implementation. CPU collection + frustum culling, plus per-frame
///        indirect args / count allocation through UploadHeapManager. Instance buffer
///        population stays stubbed — a real layout requires the upcoming material binding
///        work to know the per-instance record format.

#include "Renderer/DrawList/DrawListBuilder.h"

#include "Core/Math/Vec3.h"
#include "Renderer/Proxy/ProxyRegistry.h"
#include "Renderer/Thread/TransformDoubleBuffer.h"
#include "Renderer/Upload/UploadHeapManager.h"
#include "Renderer/View/SceneView.h"

#include <cstring>
#include <utility>

namespace Hylux::Renderer
{
namespace
{

/// @brief Size of VkDrawIndirectCommand (non-indexed). 4 uint32s: vertexCount,
///        instanceCount, firstVertex, firstInstance. Matches the backend's
///        vkCmdDrawIndirectCount mapping in VulkanCommandList.
constexpr std::uint32_t kDrawIndirectStride = sizeof(std::uint32_t) * 4;

} // namespace

DrawListBuilder::DrawListBuilder(const ProxyRegistry* proxies, TransformDoubleBuffer* transformBuffer,
                                 const SceneView* view, UploadHeapManager* uploadHeap, DrawListDesc desc) noexcept
    : proxies_(proxies), transformBuffer_(transformBuffer), view_(view), uploadHeap_(uploadHeap), desc_(std::move(desc))
{}

DrawListBuilder& DrawListBuilder::WithCustomFilter(DrawFilterFn filter)
{
    if (!filter)
    {
        return *this;
    }
    if (!filter_)
    {
        filter_ = std::move(filter);
    }
    else
    {
        auto previous = std::move(filter_);
        auto next = std::move(filter);
        filter_ = [prev = std::move(previous), nxt = std::move(next)](const PrimitiveProxy& p) {
            return prev(p) && nxt(p);
        };
    }
    return *this;
}

DrawListBuilder& DrawListBuilder::WithInstanceUpload(InstanceUploadFn writer)
{
    uploadFn_ = std::move(writer);
    return *this;
}

std::unique_ptr<DrawList> DrawListBuilder::Build()
{
    auto list = std::make_unique<DrawList>(desc_);

    const std::uint32_t transformBindless = transformBuffer_ != nullptr
                                                ? transformBuffer_->AcquireReadHalfBindlessIndex(0)
                                                : TransformDoubleBuffer::kInvalidHalfIndex;

    std::vector<ProxyId> survivors;
    if (proxies_ != nullptr)
    {
        const auto& entries = proxies_->Entries();
        survivors.reserve(entries.size());

        const bool          doFrustumCull = desc_.applyFrustumCulling && view_ != nullptr;
        const std::uint32_t cap = desc_.maxDrawCount;

        for (const auto& [id, proxy] : entries)
        {
            if ((proxy.layerMask & desc_.layerMask) == 0)
            {
                continue;
            }
            if (doFrustumCull)
            {
                const Math::Vec3 mn{proxy.bounds.minX, proxy.bounds.minY, proxy.bounds.minZ};
                const Math::Vec3 mx{proxy.bounds.maxX, proxy.bounds.maxY, proxy.bounds.maxZ};
                if (!view_->GetFrustum().IntersectsAabb(mn, mx))
                {
                    continue;
                }
            }
            if (filter_ && !filter_(proxy))
            {
                continue;
            }
            survivors.push_back(id);
            if (cap != 0 && survivors.size() >= cap)
            {
                break;
            }
        }
    }

    const auto drawCount = static_cast<std::uint32_t>(survivors.size());

    // Allocate indirect args + count from the per-frame ring when we have survivors and a
    // heap to draw from. Real per-draw values land when the mesh / material binding work
    // gives us index counts; today we zero-fill so the GPU sees "0 draws" if PSO
    // resolution somehow succeeds and DrawIndirectCount actually fires.
    RHI::IRHIBuffer* indirectArgs = nullptr;
    RHI::IRHIBuffer* indirectCount = nullptr;
    RHI::IRHIBuffer* instanceBuffer = nullptr;
    std::uint64_t    argsOffset = 0;
    std::uint64_t    countOffset = 0;
    std::uint64_t    instanceOffset = 0;
    std::uint32_t    instanceBindless = UploadHeapManager::kInvalidBindlessIndex;

    if (drawCount > 0 && uploadHeap_ != nullptr && desc_.useIndirectDraw)
    {
        const std::uint64_t argsBytes = static_cast<std::uint64_t>(drawCount) * kDrawIndirectStride;
        const auto          argsAlloc = uploadHeap_->Allocate(argsBytes, 16);
        const auto          countAlloc = uploadHeap_->Allocate(sizeof(std::uint32_t), 4);

        if (argsAlloc && countAlloc)
        {
            // VkDrawIndirectCommand per survivor. Hardcoded 3-vertex triangle until the
            // mesh asset system supplies real vertex counts. firstInstance carries the
            // proxy id so shaders can index TransformDoubleBuffer via gl_InstanceIndex
            // once bindless reads come online.
            struct DrawIndirectCommand
            {
                std::uint32_t vertexCount;
                std::uint32_t instanceCount;
                std::uint32_t firstVertex;
                std::uint32_t firstInstance;
            };
            auto* cmds = static_cast<DrawIndirectCommand*>(argsAlloc.cpu);
            for (std::uint32_t i = 0; i < drawCount; ++i)
            {
                cmds[i].vertexCount = 3;
                cmds[i].instanceCount = 1;
                cmds[i].firstVertex = 0;
                cmds[i].firstInstance = static_cast<std::uint32_t>(survivors[i]);
            }
            *static_cast<std::uint32_t*>(countAlloc.cpu) = drawCount;

            indirectArgs = argsAlloc.gpu;
            argsOffset = argsAlloc.offset;
            indirectCount = countAlloc.gpu;
            countOffset = countAlloc.offset;
        }
    }

    if (drawCount > 0 && uploadHeap_ != nullptr && desc_.instanceLayout.stride > 0)
    {
        const std::uint64_t instanceBytes = static_cast<std::uint64_t>(drawCount) * desc_.instanceLayout.stride;
        const auto          instAlloc = uploadHeap_->Allocate(instanceBytes, 16);
        if (instAlloc)
        {
            std::memset(instAlloc.cpu, 0, static_cast<std::size_t>(instanceBytes));
            // Real per-instance population happens here once the layout schema is wired
            // through MaterialProxy / PrimitiveProxy. For now we leave the bytes zeroed.
            instanceBuffer = instAlloc.gpu;
            instanceOffset = instAlloc.offset;
            instanceBindless = instAlloc.bindlessIndex;
        }
    }

    list->SetCollected(drawCount, instanceBuffer, instanceBindless, indirectArgs, indirectCount, transformBindless);
    list->SetBufferOffsets(argsOffset, countOffset, instanceOffset);
    list->SetSurvivors(std::move(survivors));
    return list;
}

} // namespace Hylux::Renderer
