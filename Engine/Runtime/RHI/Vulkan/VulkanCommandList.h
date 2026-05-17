/// @file
/// @brief Vulkan implementation of IRHICommandList. Records into a VkCommandBuffer and
///        fans debug markers out to debug-utils, the active capture tool, and the active
///        crash reporter.

#pragma once

#include "RHI/IRHICommandList.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanCommandList final : public VulkanObject, public IRHICommandList
{
public:
    VulkanCommandList(VulkanDevice* device, VulkanCommandPool* pool,
                      QueueType type, VkCommandBuffer commandBuffer);
    ~VulkanCommandList() override;

    [[nodiscard]] QueueType GetQueueType() const noexcept override { return type_; }

    bool Begin() override;
    bool End() override;
    bool Reset() override;

    void Barrier(const BarrierGroup& barriers) override;
    void BeginRendering(const RenderingInfo& info) override;
    void EndRendering() override;

    void SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline) override;
    void SetComputePipeline(IRHIComputePipeline* pipeline) override;
    void SetRayTracingPipeline(IRHIRayTracingPipeline* pipeline) override;
    void SetPipelineLayout(IRHIPipelineLayout* layout) override;

    void SetViewports(std::span<const Viewport> viewports) override;
    void SetScissors(std::span<const Rect2D> scissors) override;
    void SetBlendConstants(const float blendConstants[4]) override;
    void SetStencilReference(std::uint32_t reference) override;

    void SetVertexBuffers(std::uint32_t firstBinding,
                          std::span<const VertexBufferBinding> bindings) override;
    void SetIndexBuffer(IRHIBuffer* buffer, std::uint64_t offset, IndexType indexType) override;

    void BindBindlessHeaps(std::span<IRHIBindlessHeap* const> heaps) override;
    void SetPushConstants(std::uint32_t offset, std::uint32_t size, const void* data) override;
    void SetDescriptorSet(std::uint32_t setIndex, IRHIDescriptorSet* set) override;

    void Draw(std::uint32_t vertexCount, std::uint32_t instanceCount,
              std::uint32_t firstVertex, std::uint32_t firstInstance) override;
    void DrawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount,
                     std::uint32_t firstIndex, std::int32_t vertexOffset,
                     std::uint32_t firstInstance) override;
    void DrawIndirect(IRHIBuffer* args, std::uint64_t offset,
                      std::uint32_t drawCount, std::uint32_t stride) override;
    void DrawIndexedIndirect(IRHIBuffer* args, std::uint64_t offset,
                             std::uint32_t drawCount, std::uint32_t stride) override;
    void DrawIndirectCount(IRHIBuffer* args, std::uint64_t argsOffset,
                           IRHIBuffer* countBuffer, std::uint64_t countOffset,
                           std::uint32_t maxDrawCount, std::uint32_t stride) override;

    void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY,
                  std::uint32_t groupCountZ) override;
    void DispatchIndirect(IRHIBuffer* args, std::uint64_t offset) override;

    void DispatchMesh(std::uint32_t groupCountX, std::uint32_t groupCountY,
                      std::uint32_t groupCountZ) override;
    void DispatchMeshIndirect(IRHIBuffer* args, std::uint64_t offset,
                              std::uint32_t drawCount, std::uint32_t stride) override;

    void DispatchRays(const DispatchRaysDesc& desc) override;
    void BuildAccelerationStructures(
        std::span<const AccelerationStructureBuildDesc> builds) override;

    void CopyBuffer(IRHIBuffer* dst, std::uint64_t dstOffset,
                    IRHIBuffer* src, std::uint64_t srcOffset,
                    std::uint64_t size) override;
    void CopyBufferToTexture(IRHITexture* dst, const TextureRegion& dstRegion,
                             IRHIBuffer* src, const BufferTextureLayout& srcLayout) override;
    void CopyTextureToBuffer(IRHIBuffer* dst, const BufferTextureLayout& dstLayout,
                             IRHITexture* src, const TextureRegion& srcRegion) override;
    void CopyTexture(IRHITexture* dst, const TextureRegion& dstRegion,
                     IRHITexture* src, const TextureRegion& srcRegion) override;
    void ClearColor(IRHITextureView* view, const ClearColorValue& color) override;
    void ClearDepthStencil(IRHITextureView* view, float depth, std::uint8_t stencil) override;
    void ResolveTexture(IRHITexture* dst, IRHITexture* src, const ResolveDesc& desc) override;

    void BeginQuery(IRHIQueryPool* pool, std::uint32_t index) override;
    void EndQuery(IRHIQueryPool* pool, std::uint32_t index) override;
    void WriteTimestamp(IRHIQueryPool* pool, std::uint32_t index) override;
    void ResolveQueries(IRHIQueryPool* pool, std::uint32_t firstIndex,
                        std::uint32_t count, IRHIBuffer* dst,
                        std::uint64_t dstOffset) override;

    void PushDebugMarker(std::string_view name, std::uint32_t color = 0) override;
    void PopDebugMarker() override;
    void InsertDebugMarker(std::string_view name, std::uint32_t color = 0) override;

    [[nodiscard]] VkCommandBuffer GetVkCommandBuffer() const noexcept { return cmd_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;

    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    VulkanCommandPool* pool_{nullptr};
    QueueType          type_;
    VkCommandBuffer    cmd_{VK_NULL_HANDLE};
    bool               recording_{false};
};

} // namespace Hylux::RHI::Vulkan
