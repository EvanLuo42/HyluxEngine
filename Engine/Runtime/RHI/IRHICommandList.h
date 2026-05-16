/// @file
/// @brief Command list interface. Bindless-first recording surface covering draw, dispatch,
///        copy, sync, raytracing, query, and debug-marker commands.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIPipelineDesc.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Per-binding vertex buffer entry consumed by SetVertexBuffers.
struct VertexBufferBinding
{
    IRHIBuffer*   buffer{nullptr};
    std::uint64_t offset{0};
};

/// @brief Reference-counted handle to a command list allocated from an IRHICommandPool.
///        Not thread-safe; one command list belongs to a single recording thread at a time.
class IRHICommandList : public IRHIObject
{
public:
    /// @brief Returns the queue type the command list will be submitted to.
    [[nodiscard]] virtual QueueType GetQueueType() const noexcept = 0;

    /// @brief Opens the command list for recording. Returns false on invalid state.
    virtual bool Begin() = 0;

    /// @brief Closes the command list. Must be called before submission.
    virtual bool End() = 0;

    /// @brief Resets the command list back to the initial state without re-allocating.
    virtual bool Reset() = 0;

    /// @brief Issues a sync2-style barrier batch.
    virtual void Barrier(const BarrierGroup& barriers) = 0;

    /// @brief Begins a dynamic rendering pass with the given attachments.
    virtual void BeginRendering(const RenderingInfo& info) = 0;

    /// @brief Ends the current dynamic rendering pass.
    virtual void EndRendering() = 0;

    /// @brief Binds a graphics pipeline. Layout must match the active pipeline layout.
    virtual void SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline) = 0;

    /// @brief Binds a compute pipeline.
    virtual void SetComputePipeline(IRHIComputePipeline* pipeline) = 0;

    /// @brief Binds a ray tracing pipeline.
    virtual void SetRayTracingPipeline(IRHIRayTracingPipeline* pipeline) = 0;

    /// @brief Binds the pipeline layout used to interpret push constants and descriptor sets.
    virtual void SetPipelineLayout(IRHIPipelineLayout* layout) = 0;

    virtual void SetViewports(std::span<const Viewport> viewports) = 0;
    virtual void SetScissors(std::span<const Rect2D> scissors) = 0;
    virtual void SetBlendConstants(const float blendConstants[4]) = 0;
    virtual void SetStencilReference(std::uint32_t reference) = 0;

    virtual void SetVertexBuffers(std::uint32_t firstBinding,
                                  std::span<const VertexBufferBinding> bindings) = 0;
    virtual void SetIndexBuffer(IRHIBuffer* buffer, std::uint64_t offset, IndexType indexType) = 0;

    /// @brief Binds the bindless descriptor heap pointers active for subsequent draws.
    ///        Engine convention is to bind both kinds once at frame start; rebinding mid-frame
    ///        is supported but rare.
    virtual void BindBindlessHeaps(std::span<IRHIBindlessHeap* const> heaps) = 0;

    /// @brief Writes push constants. Offset and size are byte offsets within the push range.
    virtual void SetPushConstants(std::uint32_t offset, std::uint32_t size, const void* data) = 0;

    /// @brief Binds a classic descriptor set in the indicated layout slot. Edge-case path
    ///        used when bindless cannot represent the binding (e.g. dynamic uniform buffers).
    virtual void SetDescriptorSet(std::uint32_t setIndex, IRHIDescriptorSet* set) = 0;

    virtual void Draw(std::uint32_t vertexCount, std::uint32_t instanceCount,
                      std::uint32_t firstVertex, std::uint32_t firstInstance) = 0;
    virtual void DrawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount,
                             std::uint32_t firstIndex, std::int32_t vertexOffset,
                             std::uint32_t firstInstance) = 0;
    virtual void DrawIndirect(IRHIBuffer* args, std::uint64_t offset,
                              std::uint32_t drawCount, std::uint32_t stride) = 0;
    virtual void DrawIndexedIndirect(IRHIBuffer* args, std::uint64_t offset,
                                     std::uint32_t drawCount, std::uint32_t stride) = 0;
    virtual void DrawIndirectCount(IRHIBuffer* args, std::uint64_t argsOffset,
                                   IRHIBuffer* countBuffer, std::uint64_t countOffset,
                                   std::uint32_t maxDrawCount, std::uint32_t stride) = 0;

    virtual void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY,
                          std::uint32_t groupCountZ) = 0;
    virtual void DispatchIndirect(IRHIBuffer* args, std::uint64_t offset) = 0;

    virtual void DispatchMesh(std::uint32_t groupCountX, std::uint32_t groupCountY,
                              std::uint32_t groupCountZ) = 0;
    virtual void DispatchMeshIndirect(IRHIBuffer* args, std::uint64_t offset,
                                      std::uint32_t drawCount, std::uint32_t stride) = 0;

    virtual void DispatchRays(const DispatchRaysDesc& desc) = 0;
    virtual void BuildAccelerationStructures(
        std::span<const AccelerationStructureBuildDesc> builds) = 0;

    virtual void CopyBuffer(IRHIBuffer* dst, std::uint64_t dstOffset,
                            IRHIBuffer* src, std::uint64_t srcOffset,
                            std::uint64_t size) = 0;
    virtual void CopyBufferToTexture(IRHITexture* dst, const TextureRegion& dstRegion,
                                     IRHIBuffer* src, const BufferTextureLayout& srcLayout) = 0;
    virtual void CopyTextureToBuffer(IRHIBuffer* dst, const BufferTextureLayout& dstLayout,
                                     IRHITexture* src, const TextureRegion& srcRegion) = 0;
    virtual void CopyTexture(IRHITexture* dst, const TextureRegion& dstRegion,
                             IRHITexture* src, const TextureRegion& srcRegion) = 0;
    virtual void ClearColor(IRHITextureView* view, const ClearColorValue& color) = 0;
    virtual void ClearDepthStencil(IRHITextureView* view, float depth, std::uint8_t stencil) = 0;
    virtual void ResolveTexture(IRHITexture* dst, IRHITexture* src, const ResolveDesc& desc) = 0;

    virtual void BeginQuery(IRHIQueryPool* pool, std::uint32_t index) = 0;
    virtual void EndQuery(IRHIQueryPool* pool, std::uint32_t index) = 0;
    virtual void WriteTimestamp(IRHIQueryPool* pool, std::uint32_t index) = 0;
    virtual void ResolveQueries(IRHIQueryPool* pool, std::uint32_t firstIndex,
                                std::uint32_t count, IRHIBuffer* dst,
                                std::uint64_t dstOffset) = 0;

    /// @brief Pushes a debug region. Routes to both the native debug-utils API and the
    ///        active IGraphicsCaptureTool if one is loaded.
    virtual void PushDebugMarker(std::string_view name, std::uint32_t color = 0) = 0;

    /// @brief Pops the most recent debug region.
    virtual void PopDebugMarker() = 0;

    /// @brief Inserts a single-point debug marker.
    virtual void InsertDebugMarker(std::string_view name, std::uint32_t color = 0) = 0;
};

} // namespace Hylux::RHI
