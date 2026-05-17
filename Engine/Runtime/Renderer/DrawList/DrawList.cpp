/// @file
/// @brief DrawList implementation. Stage 4a: declarative scaffolding only.

#include "Renderer/DrawList/DrawList.h"

#include <utility>

namespace Hylux::Renderer
{

DrawList::DrawList(DrawListDesc desc) noexcept : desc_(std::move(desc)) {}

void DrawList::SetCollected(const std::uint32_t drawCount, RHI::IRHIBuffer* instanceBuffer,
                            const std::uint32_t instanceBindlessIndex, RHI::IRHIBuffer* indirectArgs,
                            RHI::IRHIBuffer* indirectCount, const std::uint32_t transformBindlessIndex) noexcept
{
    drawCount_ = drawCount;
    instanceBuffer_ = instanceBuffer;
    instanceBindlessIndex_ = instanceBindlessIndex;
    indirectArgs_ = indirectArgs;
    indirectCount_ = indirectCount;
    transformBindlessIndex_ = transformBindlessIndex;
}

} // namespace Hylux::Renderer
