/// @file
/// @brief PipelineLayoutCache implementation.

#include "Renderer/Pso/PipelineLayoutCache.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIPipelineDesc.h"

namespace Hylux::Renderer
{

PipelineLayoutCache::PipelineLayoutCache(RHI::IRHIDevice* device) noexcept : device_(device) {}

RHI::IRHIPipelineLayout* PipelineLayoutCache::GetOrCreate(std::uint64_t                  layoutHash,
                                                          const Shader::ShaderReflection& reflection)
{
    if (auto it = entries_.find(layoutHash); it != entries_.end())
    {
        return it->second.Get();
    }
    if (device_ == nullptr)
    {
        return nullptr;
    }

    RHI::PipelineLayoutDesc desc{};
    desc.pushConstantSize        = reflection.pushConstantSize;
    desc.pushConstantVisibility  = static_cast<RHI::ShaderStage>(reflection.pushConstantVisibility);
    desc.enableBindlessSrvCbvUav = true;
    desc.enableBindlessSampler   = true;

    auto layout = device_->CreatePipelineLayout(desc);
    if (!layout)
    {
        HYLUX_LOG_ERROR(LogRender,
                        "PipelineLayoutCache: CreatePipelineLayout failed (layoutHash={:#x})",
                        layoutHash);
        return nullptr;
    }
    auto* raw = layout.Get();
    entries_.emplace(layoutHash, std::move(layout));
    return raw;
}

void PipelineLayoutCache::Clear() noexcept
{
    entries_.clear();
}

} // namespace Hylux::Renderer
