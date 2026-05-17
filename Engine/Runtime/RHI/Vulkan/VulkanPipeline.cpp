/// @file
/// @brief Minimal pipeline implementations. Compute pipelines are fully functional;
///        graphics + raytracing pipelines build VkPipeline handles via the standard chain
///        but advanced state (multi-attachment blending, tessellation, complex RT SBT
///        layout) is left as future work — the create infos compile but only the
///        common-path fields are populated.

#include "RHI/Vulkan/VulkanPipeline.h"

#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"
#include "RHI/Vulkan/VulkanFormat.h"
#include "RHI/Vulkan/VulkanShaderModule.h"

#include <vector>

namespace Hylux::RHI::Vulkan
{

// VulkanPipelineLayout --------------------------------------------------------------------

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    VkPushConstantRange pc{};
    pc.stageFlags = ToVkShaderStageFlags(desc.pushConstantVisibility);
    pc.offset     = 0;
    pc.size       = desc.pushConstantSize;

    VkPipelineLayoutCreateInfo ci{};
    ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ci.pushConstantRangeCount = desc.pushConstantSize > 0 ? 1 : 0;
    ci.pPushConstantRanges    = desc.pushConstantSize > 0 ? &pc : nullptr;
    // Descriptor set layouts: not wired up here (bindless is the primary path).

    if (vkCreatePipelineLayout(device->GetVkDevice(), &ci, nullptr, &layout_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreatePipelineLayout failed");
        layout_ = VK_NULL_HANDLE;
    }
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    if (layout_ != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_->GetVkDevice(), layout_, nullptr);
}

RHINativeHandle VulkanPipelineLayout::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPipelineLayout,
                           reinterpret_cast<std::uint64_t>(layout_)};
}

void VulkanPipelineLayout::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                      reinterpret_cast<std::uint64_t>(layout_), debugName_);
}

// VulkanGraphicsPipeline ------------------------------------------------------------------

namespace
{

std::vector<VkPipelineShaderStageCreateInfo> MakeGraphicsStages(const GraphicsPipelineDesc& d)
{
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    auto add = [&](IRHIShaderModule* m, VkShaderStageFlagBits stage) {
        if (!m) return;
        VkPipelineShaderStageCreateInfo s{};
        s.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s.stage  = stage;
        s.module = static_cast<VulkanShaderModule*>(m)->GetVkShaderModule();
        s.pName  = m->GetEntryPoint().data();  // SPIR-V entry names are NUL-terminated in our cache.
        stages.push_back(s);
    };
    add(d.vertexShader,    VK_SHADER_STAGE_VERTEX_BIT);
    add(d.hullShader,      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    add(d.domainShader,    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    add(d.geometryShader,  VK_SHADER_STAGE_GEOMETRY_BIT);
    add(d.pixelShader,     VK_SHADER_STAGE_FRAGMENT_BIT);
    add(d.taskShader,      VK_SHADER_STAGE_TASK_BIT_EXT);
    add(d.meshShader,      VK_SHADER_STAGE_MESH_BIT_EXT);
    return stages;
}

} // namespace

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice* device, const GraphicsPipelineDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    bindPoint_ = VK_PIPELINE_BIND_POINT_GRAPHICS;

    auto stages = MakeGraphicsStages(desc);

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = ToVkTopology(desc.topology);

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = ToVkPolygonMode(desc.rasterizer.fillMode);
    rs.cullMode    = ToVkCullMode(desc.rasterizer.cullMode);
    rs.frontFace   = ToVkFrontFace(desc.rasterizer.frontFace);
    rs.lineWidth   = 1.0f;
    rs.depthBiasEnable         = desc.rasterizer.depthBiasEnable ? VK_TRUE : VK_FALSE;
    rs.depthBiasConstantFactor = desc.rasterizer.depthBias;
    rs.depthBiasClamp          = desc.rasterizer.depthBiasClamp;
    rs.depthBiasSlopeFactor    = desc.rasterizer.depthBiasSlope;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = ToVkSampleCount(desc.sampleCount);

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = desc.depthStencil.depthTestEnable  ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = desc.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp   = ToVkCompareOp(desc.depthStencil.depthCompareOp);
    ds.stencilTestEnable= desc.depthStencil.stencilTestEnable ? VK_TRUE : VK_FALSE;

    std::vector<VkPipelineColorBlendAttachmentState> blends;
    std::vector<VkFormat>                            colorFormats;
    for (std::uint32_t i = 0; i < kMaxColorAttachments; ++i)
    {
        if (desc.colorFormats[i] == Format::Unknown) continue;
        const auto& a = desc.blend.attachments[i];
        VkPipelineColorBlendAttachmentState s{};
        s.blendEnable         = a.blendEnable ? VK_TRUE : VK_FALSE;
        s.srcColorBlendFactor = ToVkBlendFactor(a.srcColor);
        s.dstColorBlendFactor = ToVkBlendFactor(a.dstColor);
        s.colorBlendOp        = ToVkBlendOp(a.colorOp);
        s.srcAlphaBlendFactor = ToVkBlendFactor(a.srcAlpha);
        s.dstAlphaBlendFactor = ToVkBlendFactor(a.dstAlpha);
        s.alphaBlendOp        = ToVkBlendOp(a.alphaOp);
        s.colorWriteMask      = a.writeMask;
        blends.push_back(s);
        colorFormats.push_back(ToVkFormat(desc.colorFormats[i]));
    }
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = static_cast<std::uint32_t>(blends.size());
    cb.pAttachments    = blends.data();

    VkDynamicState dyn[3] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                             VK_DYNAMIC_STATE_BLEND_CONSTANTS};
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = 3;
    dynState.pDynamicStates    = dyn;

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount    = static_cast<std::uint32_t>(colorFormats.size());
    rendering.pColorAttachmentFormats = colorFormats.data();
    rendering.depthAttachmentFormat   = ToVkFormat(desc.depthStencilFormat);

    VkPipelineLayout layout = desc.layout
        ? static_cast<VulkanPipelineLayout*>(desc.layout)->GetVkPipelineLayout()
        : VK_NULL_HANDLE;

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.pNext               = &rendering;
    ci.stageCount          = static_cast<std::uint32_t>(stages.size());
    ci.pStages             = stages.data();
    ci.pVertexInputState   = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState      = &vp;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState   = &ms;
    ci.pDepthStencilState  = &ds;
    ci.pColorBlendState    = &cb;
    ci.pDynamicState       = &dynState;
    ci.layout              = layout;

    if (vkCreateGraphicsPipelines(device->GetVkDevice(), VK_NULL_HANDLE, 1, &ci,
                                  nullptr, &pipeline_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateGraphicsPipelines failed");
        pipeline_ = VK_NULL_HANDLE;
    }
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (pipeline_ != VK_NULL_HANDLE)
        vkDestroyPipeline(device_->GetVkDevice(), pipeline_, nullptr);
}

RHINativeHandle VulkanGraphicsPipeline::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPipeline,
                           reinterpret_cast<std::uint64_t>(pipeline_)};
}

void VulkanGraphicsPipeline::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_PIPELINE,
                      reinterpret_cast<std::uint64_t>(pipeline_), debugName_);
}

// VulkanComputePipeline -------------------------------------------------------------------

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* device, const ComputePipelineDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    bindPoint_ = VK_PIPELINE_BIND_POINT_COMPUTE;

    if (!desc.computeShader)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "ComputePipeline: computeShader is null");
        return;
    }
    auto* cs = static_cast<VulkanShaderModule*>(desc.computeShader);

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = cs->GetVkShaderModule();
    stage.pName  = cs->GetEntryPoint().data();

    VkComputePipelineCreateInfo ci{};
    ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.stage  = stage;
    ci.layout = desc.layout
        ? static_cast<VulkanPipelineLayout*>(desc.layout)->GetVkPipelineLayout()
        : VK_NULL_HANDLE;

    if (vkCreateComputePipelines(device->GetVkDevice(), VK_NULL_HANDLE, 1, &ci,
                                 nullptr, &pipeline_) != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkCreateComputePipelines failed");
        pipeline_ = VK_NULL_HANDLE;
    }
}

VulkanComputePipeline::~VulkanComputePipeline()
{
    if (pipeline_ != VK_NULL_HANDLE)
        vkDestroyPipeline(device_->GetVkDevice(), pipeline_, nullptr);
}

RHINativeHandle VulkanComputePipeline::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPipeline,
                           reinterpret_cast<std::uint64_t>(pipeline_)};
}

void VulkanComputePipeline::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_PIPELINE,
                      reinterpret_cast<std::uint64_t>(pipeline_), debugName_);
}

// VulkanRayTracingPipeline ----------------------------------------------------------------
// Minimal stub: TODO full RT SBT construction. Just creates an empty pipeline if RT is
// enabled so the device's CreateRayTracingPipeline returns a non-null Ref.

VulkanRayTracingPipeline::VulkanRayTracingPipeline(VulkanDevice* device, const RayTracingPipelineDesc& desc)
    : VulkanObject(device), desc_(desc)
{
    bindPoint_ = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    HYLUX_LOG(::Hylux::LogRender, Warn, "VulkanRayTracingPipeline: stub implementation");
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
{
    if (pipeline_ != VK_NULL_HANDLE)
        vkDestroyPipeline(device_->GetVkDevice(), pipeline_, nullptr);
}

RHINativeHandle VulkanRayTracingPipeline::GetNativeHandle(NativeHandleQuery /*q*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkPipeline,
                           reinterpret_cast<std::uint64_t>(pipeline_)};
}

void VulkanRayTracingPipeline::OnDebugNameChanged() {}

} // namespace Hylux::RHI::Vulkan
