/// @file
/// @brief Descriptor structs for pipeline state, pipeline layout, and dynamic rendering.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Single vertex input element description used inside VertexInputDesc.
struct VertexAttribute
{
    std::uint32_t   location{0};
    std::uint32_t   binding{0};
    Format          format{Format::Unknown};
    std::uint32_t   offset{0};
};

/// @brief Per-binding vertex stream description used inside VertexInputDesc.
struct VertexBinding
{
    std::uint32_t   binding{0};
    std::uint32_t   stride{0};
    bool            perInstance{false};
};

/// @brief Aggregate vertex input layout description.
struct VertexInputDesc
{
    std::span<const VertexBinding>   bindings{};
    std::span<const VertexAttribute> attributes{};
};

/// @brief Rasterizer state.
struct RasterizerDesc
{
    FillMode   fillMode{FillMode::Solid};
    CullMode   cullMode{CullMode::Back};
    FrontFace  frontFace{FrontFace::CounterClockwise};
    bool       depthClampEnable{false};
    bool       depthBiasEnable{false};
    float      depthBias{0.0f};
    float      depthBiasClamp{0.0f};
    float      depthBiasSlope{0.0f};
    bool       conservativeRasterization{false};
};

/// @brief Per-side stencil state.
struct StencilState
{
    StencilOp  failOp{StencilOp::Keep};
    StencilOp  passOp{StencilOp::Keep};
    StencilOp  depthFailOp{StencilOp::Keep};
    CompareOp  compareOp{CompareOp::Always};
    std::uint8_t readMask{0xFF};
    std::uint8_t writeMask{0xFF};
};

/// @brief Depth-stencil pipeline state.
struct DepthStencilDesc
{
    bool         depthTestEnable{false};
    bool         depthWriteEnable{false};
    CompareOp    depthCompareOp{CompareOp::Less};
    bool         stencilTestEnable{false};
    StencilState frontStencil{};
    StencilState backStencil{};
};

/// @brief Per-attachment color blend state.
struct ColorAttachmentBlend
{
    bool        blendEnable{false};
    BlendFactor srcColor{BlendFactor::One};
    BlendFactor dstColor{BlendFactor::Zero};
    BlendOp     colorOp{BlendOp::Add};
    BlendFactor srcAlpha{BlendFactor::One};
    BlendFactor dstAlpha{BlendFactor::Zero};
    BlendOp     alphaOp{BlendOp::Add};
    std::uint8_t writeMask{0xF};
};

/// @brief Aggregate blend state across all color attachments.
struct BlendDesc
{
    std::array<ColorAttachmentBlend, kMaxColorAttachments> attachments{};
    float blendConstants[4]{0.0f, 0.0f, 0.0f, 0.0f};
    bool  alphaToCoverageEnable{false};
};

/// @brief Pipeline layout description. Bindless-first: push constant range + the per-stage
///        visibility for bindless heap pointers are baked in. Explicit descriptor set
///        layouts remain available for the edge-case binding path.
struct PipelineLayoutDesc
{
    std::uint32_t pushConstantSize{0};
    ShaderStage   pushConstantVisibility{ShaderStage::All};
    std::span<IRHIDescriptorSetLayout* const> descriptorSetLayouts{};
    bool          enableBindlessSrvCbvUav{true};
    bool          enableBindlessSampler{true};
};

/// @brief Graphics pipeline description. Dynamic rendering (no render pass object) is the
///        only supported binding mode; render target formats are baked into the pipeline.
struct GraphicsPipelineDesc
{
    IRHIPipelineLayout*  layout{nullptr};
    IRHIShaderModule*    vertexShader{nullptr};
    IRHIShaderModule*    pixelShader{nullptr};
    IRHIShaderModule*    hullShader{nullptr};
    IRHIShaderModule*    domainShader{nullptr};
    IRHIShaderModule*    geometryShader{nullptr};
    IRHIShaderModule*    taskShader{nullptr};
    IRHIShaderModule*    meshShader{nullptr};

    VertexInputDesc      vertexInput{};
    PrimitiveTopology    topology{PrimitiveTopology::TriangleList};
    RasterizerDesc       rasterizer{};
    DepthStencilDesc     depthStencil{};
    BlendDesc            blend{};

    std::array<Format, kMaxColorAttachments> colorFormats{};
    Format               depthStencilFormat{Format::Unknown};
    std::uint32_t        sampleCount{1};
    std::uint32_t        viewMask{0};
};

/// @brief Compute pipeline description.
struct ComputePipelineDesc
{
    IRHIPipelineLayout* layout{nullptr};
    IRHIShaderModule*   computeShader{nullptr};
};

/// @brief Ray tracing shader bundle entry used inside RayTracingPipelineDesc.
struct RayTracingShader
{
    IRHIShaderModule* module{nullptr};
    ShaderStage       stage{ShaderStage::None};
    std::string_view  entryPoint{};
};

/// @brief Ray tracing hit group description.
struct HitGroupDesc
{
    std::string_view name{};
    std::uint32_t    closestHitIndex{0xFFFFFFFFu};
    std::uint32_t    anyHitIndex{0xFFFFFFFFu};
    std::uint32_t    intersectionIndex{0xFFFFFFFFu};
};

/// @brief Ray tracing pipeline description.
struct RayTracingPipelineDesc
{
    IRHIPipelineLayout*              layout{nullptr};
    std::span<const RayTracingShader> shaders{};
    std::span<const HitGroupDesc>     hitGroups{};
    std::uint32_t                     maxRecursionDepth{1};
    std::uint32_t                     maxPayloadSize{16};
    std::uint32_t                     maxAttributeSize{8};
};

/// @brief Single attachment record inside RenderingInfo for dynamic rendering.
struct RenderingAttachment
{
    IRHITextureView* view{nullptr};
    ImageLayout      layout{ImageLayout::ColorAttachment};
    LoadOp           loadOp{LoadOp::Load};
    StoreOp          storeOp{StoreOp::Store};
    ClearValue       clearValue{};
    IRHITextureView* resolveView{nullptr};
    ImageLayout      resolveLayout{ImageLayout::Undefined};
};

/// @brief Aggregate rendering pass description for IRHICommandList::BeginRendering.
struct RenderingInfo
{
    std::span<const RenderingAttachment> colorAttachments{};
    std::optional<RenderingAttachment>   depthAttachment{};
    std::optional<RenderingAttachment>   stencilAttachment{};
    Rect2D                               renderArea{};
    std::uint32_t                        layerCount{1};
    std::uint32_t                        viewMask{0};
};

/// @brief Indirect dispatch / draw argument layouts. Exposed for reference; backends know
///        the GPU-side layout matches D3D12 indirect arg struct conventions.
struct DispatchIndirectArgs
{
    std::uint32_t threadGroupCountX{0};
    std::uint32_t threadGroupCountY{0};
    std::uint32_t threadGroupCountZ{0};
};

/// @brief Indirect ray dispatch description.
struct DispatchRaysDesc
{
    IRHIBuffer*    rayGenShaderTable{nullptr};
    std::uint64_t  rayGenOffset{0};
    std::uint64_t  rayGenSize{0};
    IRHIBuffer*    missShaderTable{nullptr};
    std::uint64_t  missOffset{0};
    std::uint64_t  missSize{0};
    std::uint64_t  missStride{0};
    IRHIBuffer*    hitGroupTable{nullptr};
    std::uint64_t  hitGroupOffset{0};
    std::uint64_t  hitGroupSize{0};
    std::uint64_t  hitGroupStride{0};
    IRHIBuffer*    callableTable{nullptr};
    std::uint64_t  callableOffset{0};
    std::uint64_t  callableSize{0};
    std::uint64_t  callableStride{0};
    std::uint32_t  width{0};
    std::uint32_t  height{0};
    std::uint32_t  depth{1};
};

} // namespace Hylux::RHI
