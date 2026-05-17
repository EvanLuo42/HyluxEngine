/// @file
/// @brief Implementation of the IRHIDevice resource-creation methods. Kept separate from
///        VulkanDevice.cpp so the device translation unit isn't dominated by per-resource
///        boilerplate.

#include "RHI/Vulkan/VulkanDevice.h"

#include "RHI/Vulkan/VulkanAccelerationStructure.h"
#include "RHI/Vulkan/VulkanBindlessHeap.h"
#include "RHI/Vulkan/VulkanBuffer.h"
#include "RHI/Vulkan/VulkanDescriptorSet.h"
#include "RHI/Vulkan/VulkanPipeline.h"
#include "RHI/Vulkan/VulkanPipelineCache.h"
#include "RHI/Vulkan/VulkanQueryPool.h"
#include "RHI/Vulkan/VulkanSampler.h"
#include "RHI/Vulkan/VulkanShaderModule.h"
#include "RHI/Vulkan/VulkanSurface.h"
#include "RHI/Vulkan/VulkanTexture.h"
#include "RHI/Vulkan/VulkanVendorExtension.h"

namespace Hylux::RHI::Vulkan
{

Ref<IRHIBuffer> VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    auto b = MakeRef<VulkanBuffer>(this, desc);
    return b && b->IsValid() ? Ref<IRHIBuffer>(b) : Ref<IRHIBuffer>{};
}

Ref<IRHITexture> VulkanDevice::CreateTexture(const TextureDesc& desc)
{
    auto t = MakeRef<VulkanTexture>(this, desc);
    return t && t->IsValid() ? Ref<IRHITexture>(t) : Ref<IRHITexture>{};
}

Ref<IRHITextureView> VulkanDevice::CreateTextureView(IRHITexture* texture,
                                                     const TextureViewDesc& desc)
{
    if (!texture) return {};
    auto v = MakeRef<VulkanTextureView>(this, static_cast<VulkanTexture*>(texture), desc);
    return v && v->IsValid() ? Ref<IRHITextureView>(v) : Ref<IRHITextureView>{};
}

Ref<IRHISampler> VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    auto s = MakeRef<VulkanSampler>(this, desc);
    return s && s->IsValid() ? Ref<IRHISampler>(s) : Ref<IRHISampler>{};
}

Ref<IRHIShaderModule> VulkanDevice::CreateShaderModule(const ShaderBytecode& bytecode)
{
    auto sm = MakeRef<VulkanShaderModule>(this, bytecode);
    return sm && sm->IsValid() ? Ref<IRHIShaderModule>(sm) : Ref<IRHIShaderModule>{};
}

Ref<IRHIPipelineLayout> VulkanDevice::CreatePipelineLayout(const PipelineLayoutDesc& desc)
{
    auto l = MakeRef<VulkanPipelineLayout>(this, desc);
    return l && l->IsValid() ? Ref<IRHIPipelineLayout>(l) : Ref<IRHIPipelineLayout>{};
}

Ref<IRHIGraphicsPipeline> VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    auto p = MakeRef<VulkanGraphicsPipeline>(this, desc);
    return p && p->IsValid() ? Ref<IRHIGraphicsPipeline>(p) : Ref<IRHIGraphicsPipeline>{};
}

Ref<IRHIComputePipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    auto p = MakeRef<VulkanComputePipeline>(this, desc);
    return p && p->IsValid() ? Ref<IRHIComputePipeline>(p) : Ref<IRHIComputePipeline>{};
}

Ref<IRHIRayTracingPipeline> VulkanDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    auto p = MakeRef<VulkanRayTracingPipeline>(this, desc);
    return Ref<IRHIRayTracingPipeline>(p);
}

Ref<IRHIAccelerationStructure> VulkanDevice::CreateBlas(const BlasDesc& /*desc*/)
{
    return MakeRef<VulkanAccelerationStructure>(this);
}

Ref<IRHIAccelerationStructure> VulkanDevice::CreateTlas(const TlasDesc& /*desc*/)
{
    return MakeRef<VulkanAccelerationStructure>(this);
}

Ref<IRHIPipelineCache> VulkanDevice::GetOrCreatePipelineCache(std::string_view name)
{
    const std::string key(name);
    if (auto it = pipelineCaches_.find(key); it != pipelineCaches_.end())
    {
        return it->second;
    }
    auto cache = MakeRef<VulkanPipelineCache>(this);
    pipelineCaches_.emplace(key, cache);
    return cache;
}

Ref<IRHIBindlessHeap> VulkanDevice::GetBindlessHeap(BindlessKind kind)
{
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= bindlessHeaps_.size()) return {};
    if (!bindlessHeaps_[idx])
    {
        const std::uint32_t cap = (kind == BindlessKind::Sampler)
            ? desc_.bindlessSizes.samplerCapacity
            : desc_.bindlessSizes.srvCbvUavCapacity;
        bindlessHeaps_[idx] = MakeRef<VulkanBindlessHeap>(this, kind, cap);
    }
    return bindlessHeaps_[idx];
}

Ref<IRHIQueryPool> VulkanDevice::CreateQueryPool(QueryType type, std::uint32_t count)
{
    auto q = MakeRef<VulkanQueryPool>(this, type, count);
    return q && q->IsValid() ? Ref<IRHIQueryPool>(q) : Ref<IRHIQueryPool>{};
}

Ref<IRHISurface> VulkanDevice::CreateSurface(const PlatformWindowHandle& window)
{
    auto s = MakeRef<VulkanSurface>(this, window);
    return s && s->IsValid() ? Ref<IRHISurface>(s) : Ref<IRHISurface>{};
}

Ref<IRHISwapchain> VulkanDevice::CreateSwapchain(IRHISurface* surface, const SwapchainDesc& desc)
{
    if (!surface) return {};
    auto sc = MakeRef<VulkanSwapchain>(this, static_cast<VulkanSurface*>(surface), desc);
    return sc && sc->IsValid() ? Ref<IRHISwapchain>(sc) : Ref<IRHISwapchain>{};
}

Ref<IRHIDescriptorSetLayout> VulkanDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc)
{
    auto l = MakeRef<VulkanDescriptorSetLayout>(this, desc);
    return l && l->IsValid() ? Ref<IRHIDescriptorSetLayout>(l) : Ref<IRHIDescriptorSetLayout>{};
}

Ref<IRHIDescriptorSet> VulkanDevice::AllocateDescriptorSet(IRHIDescriptorSetLayout* /*layout*/)
{
    // Explicit descriptor sets are an edge-case binding path; bindless heaps handle the
    // typical render frame. Returning null until the engine actually exercises this path.
    return {};
}

Ref<IRHIVendorExtension> VulkanDevice::QueryVendorExtension(VendorExtensionKind /*kind*/)
{
    return {};
}

} // namespace Hylux::RHI::Vulkan
