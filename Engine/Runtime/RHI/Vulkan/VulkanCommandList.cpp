/// @file
/// @brief VulkanCommandList implementation. Encodes draws/dispatches/copies and fans debug
///        markers out to debug-utils, the active IGraphicsCaptureTool, and the active
///        IGpuCrashReporter.

#include "RHI/Vulkan/VulkanCommandList.h"

#include "Core/Containers/SmallVector.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/Diagnostics/IGpuCrashReporter.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHIInstance.h"
#include "RHI/Vulkan/VulkanBuffer.h"
#include "RHI/Vulkan/VulkanCommandPool.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"
#include "RHI/Vulkan/VulkanFormat.h"
#include "RHI/Vulkan/VulkanPipeline.h"
#include "RHI/Vulkan/VulkanTexture.h"

#include <atomic>
#include <cstring>

namespace Hylux::RHI::Vulkan
{

namespace
{

VkImageSubresourceLayers ToVkSubresourceLayers(const SubresourceRange& r, Format format)
{
    VkImageSubresourceLayers out{};
    out.aspectMask     = FormatAspect(format);
    out.mipLevel       = r.baseMipLevel;
    out.baseArrayLayer = r.baseArrayLayer;
    out.layerCount     = r.arrayLayerCount;
    return out;
}

void DecodeMarkerColor(std::uint32_t color, float (&out)[4]) noexcept
{
    if (color == 0) { out[0]=out[1]=out[2]=out[3]=1.0f; return; }
    out[0] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    out[1] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    out[2] = static_cast<float>((color >>  8) & 0xFF) / 255.0f;
    out[3] = static_cast<float>((color      ) & 0xFF) / 255.0f;
}

} // namespace

VulkanCommandList::VulkanCommandList(VulkanDevice* device, VulkanCommandPool* pool,
                                     QueueType type, VkCommandBuffer cb)
    : VulkanObject(device), pool_(pool), type_(type), cmd_(cb) {}

VulkanCommandList::~VulkanCommandList()
{
    if (cmd_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device_->GetVkDevice(), pool_->GetVkCommandPool(), 1, &cmd_);
    }
}

bool VulkanCommandList::Begin()
{
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd_, &bi) != VK_SUCCESS) return false;
    recording_ = true;
    return true;
}

bool VulkanCommandList::End()
{
    if (!recording_) return false;
    recording_ = false;
    return vkEndCommandBuffer(cmd_) == VK_SUCCESS;
}

bool VulkanCommandList::Reset()
{
    recording_ = false;
    return vkResetCommandBuffer(cmd_, 0) == VK_SUCCESS;
}

void VulkanCommandList::Barrier(const BarrierGroup& barriers)
{
    SmallVector<VkMemoryBarrier2, 4>       mem;
    SmallVector<VkBufferMemoryBarrier2, 8> buf;
    SmallVector<VkImageMemoryBarrier2, 8>  img;
    mem.Reserve(barriers.memory.size());
    buf.Reserve(barriers.buffers.size());
    img.Reserve(barriers.textures.size());

    for (const auto& m : barriers.memory)
    {
        VkMemoryBarrier2 b{};
        b.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        b.srcStageMask  = ToVkPipelineStages(m.srcStages);
        b.srcAccessMask = ToVkAccessFlags(m.srcAccess);
        b.dstStageMask  = ToVkPipelineStages(m.dstStages);
        b.dstAccessMask = ToVkAccessFlags(m.dstAccess);
        mem.PushBack(b);
    }
    for (const auto& bb : barriers.buffers)
    {
        VkBufferMemoryBarrier2 b{};
        b.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        b.srcStageMask        = ToVkPipelineStages(bb.srcStages);
        b.srcAccessMask       = ToVkAccessFlags(bb.srcAccess);
        b.dstStageMask        = ToVkPipelineStages(bb.dstStages);
        b.dstAccessMask       = ToVkAccessFlags(bb.dstAccess);
        b.srcQueueFamilyIndex = bb.srcQueueFamilyIndex == kQueueFamilyIgnored ? VK_QUEUE_FAMILY_IGNORED : bb.srcQueueFamilyIndex;
        b.dstQueueFamilyIndex = bb.dstQueueFamilyIndex == kQueueFamilyIgnored ? VK_QUEUE_FAMILY_IGNORED : bb.dstQueueFamilyIndex;
        b.buffer              = static_cast<VulkanBuffer*>(bb.buffer)->GetVkBuffer();
        b.offset              = bb.offset;
        b.size                = bb.size;
        buf.PushBack(b);
    }
    for (const auto& tb : barriers.textures)
    {
        auto* tex = static_cast<VulkanTexture*>(tb.texture);
        VkImageMemoryBarrier2 b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        b.srcStageMask        = ToVkPipelineStages(tb.srcStages);
        b.srcAccessMask       = ToVkAccessFlags(tb.srcAccess);
        b.dstStageMask        = ToVkPipelineStages(tb.dstStages);
        b.dstAccessMask       = ToVkAccessFlags(tb.dstAccess);
        b.oldLayout           = ToVkImageLayout(tb.oldLayout);
        b.newLayout           = ToVkImageLayout(tb.newLayout);
        b.srcQueueFamilyIndex = tb.srcQueueFamilyIndex == kQueueFamilyIgnored ? VK_QUEUE_FAMILY_IGNORED : tb.srcQueueFamilyIndex;
        b.dstQueueFamilyIndex = tb.dstQueueFamilyIndex == kQueueFamilyIgnored ? VK_QUEUE_FAMILY_IGNORED : tb.dstQueueFamilyIndex;
        b.image               = tex->GetVkImage();
        b.subresourceRange.aspectMask     = FormatAspect(tex->GetDesc().format);
        b.subresourceRange.baseMipLevel   = tb.range.baseMipLevel;
        b.subresourceRange.levelCount     = tb.range.mipLevelCount;
        b.subresourceRange.baseArrayLayer = tb.range.baseArrayLayer;
        b.subresourceRange.layerCount     = tb.range.arrayLayerCount;
        img.PushBack(b);
    }

    VkDependencyInfo dep{};
    dep.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.memoryBarrierCount       = static_cast<std::uint32_t>(mem.size());
    dep.pMemoryBarriers          = mem.data();
    dep.bufferMemoryBarrierCount = static_cast<std::uint32_t>(buf.size());
    dep.pBufferMemoryBarriers    = buf.data();
    dep.imageMemoryBarrierCount  = static_cast<std::uint32_t>(img.size());
    dep.pImageMemoryBarriers     = img.data();

    vkCmdPipelineBarrier2(cmd_, &dep);
}

void VulkanCommandList::BeginRendering(const RenderingInfo& info)
{
    SmallVector<VkRenderingAttachmentInfo, 8> colorAtts;
    colorAtts.Reserve(info.colorAttachments.size());
    for (const auto& a : info.colorAttachments)
    {
        VkRenderingAttachmentInfo va{};
        va.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        va.imageView   = a.view ? static_cast<VulkanTextureView*>(a.view)->GetVkImageView() : VK_NULL_HANDLE;
        va.imageLayout = ToVkImageLayout(a.layout);
        va.loadOp      = ToVkLoadOp(a.loadOp);
        va.storeOp     = ToVkStoreOp(a.storeOp);
        va.clearValue.color.float32[0] = a.clearValue.color.r;
        va.clearValue.color.float32[1] = a.clearValue.color.g;
        va.clearValue.color.float32[2] = a.clearValue.color.b;
        va.clearValue.color.float32[3] = a.clearValue.color.a;
        colorAtts.PushBack(va);
    }

    VkRenderingAttachmentInfo depthAtt{};
    if (info.depthAttachment.has_value())
    {
        const auto& a = *info.depthAttachment;
        depthAtt.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAtt.imageView   = a.view ? static_cast<VulkanTextureView*>(a.view)->GetVkImageView() : VK_NULL_HANDLE;
        depthAtt.imageLayout = ToVkImageLayout(a.layout);
        depthAtt.loadOp      = ToVkLoadOp(a.loadOp);
        depthAtt.storeOp     = ToVkStoreOp(a.storeOp);
        depthAtt.clearValue.depthStencil.depth   = a.clearValue.depthStencil.depth;
        depthAtt.clearValue.depthStencil.stencil = a.clearValue.depthStencil.stencil;
    }

    VkRenderingInfo ri{};
    ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea.offset.x  = info.renderArea.offset.x;
    ri.renderArea.offset.y  = info.renderArea.offset.y;
    ri.renderArea.extent.width  = info.renderArea.extent.width;
    ri.renderArea.extent.height = info.renderArea.extent.height;
    ri.layerCount           = info.layerCount;
    ri.viewMask             = info.viewMask;
    ri.colorAttachmentCount = static_cast<std::uint32_t>(colorAtts.size());
    ri.pColorAttachments    = colorAtts.data();
    if (info.depthAttachment.has_value())     ri.pDepthAttachment = &depthAtt;
    vkCmdBeginRendering(cmd_, &ri);
}

void VulkanCommandList::EndRendering()                        { vkCmdEndRendering(cmd_); }

void VulkanCommandList::SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline)
{
    auto* p = static_cast<VulkanGraphicsPipeline*>(pipeline);
    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, p->GetVkPipeline());
}

void VulkanCommandList::SetComputePipeline(IRHIComputePipeline* pipeline)
{
    auto* p = static_cast<VulkanComputePipeline*>(pipeline);
    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, p->GetVkPipeline());
}

void VulkanCommandList::SetRayTracingPipeline(IRHIRayTracingPipeline* pipeline)
{
    auto* p = static_cast<VulkanRayTracingPipeline*>(pipeline);
    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, p->GetVkPipeline());
}

void VulkanCommandList::SetPipelineLayout(IRHIPipelineLayout* /*layout*/) {}

void VulkanCommandList::SetViewports(std::span<const Viewport> viewports)
{
    SmallVector<VkViewport, 4> vp;
    vp.Reserve(viewports.size());
    for (const auto& v : viewports)
    {
        VkViewport x{};
        x.x = v.x; x.y = v.y; x.width = v.width; x.height = v.height;
        x.minDepth = v.minDepth; x.maxDepth = v.maxDepth;
        vp.PushBack(x);
    }
    vkCmdSetViewport(cmd_, 0, static_cast<std::uint32_t>(vp.size()), vp.data());
}

void VulkanCommandList::SetScissors(std::span<const Rect2D> scissors)
{
    SmallVector<VkRect2D, 4> rs;
    rs.Reserve(scissors.size());
    for (const auto& s : scissors)
    {
        VkRect2D r{};
        r.offset.x = s.offset.x; r.offset.y = s.offset.y;
        r.extent.width = s.extent.width; r.extent.height = s.extent.height;
        rs.PushBack(r);
    }
    vkCmdSetScissor(cmd_, 0, static_cast<std::uint32_t>(rs.size()), rs.data());
}

void VulkanCommandList::SetBlendConstants(const float c[4])
{
    vkCmdSetBlendConstants(cmd_, c);
}

void VulkanCommandList::SetStencilReference(std::uint32_t reference)
{
    vkCmdSetStencilReference(cmd_, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
}

void VulkanCommandList::SetVertexBuffers(std::uint32_t firstBinding,
                                         std::span<const VertexBufferBinding> bindings)
{
    SmallVector<VkBuffer, 4>     bufs;
    SmallVector<VkDeviceSize, 4> offs;
    bufs.Reserve(bindings.size());
    offs.Reserve(bindings.size());
    for (const auto& b : bindings)
    {
        bufs.PushBack(static_cast<VulkanBuffer*>(b.buffer)->GetVkBuffer());
        offs.PushBack(b.offset);
    }
    vkCmdBindVertexBuffers(cmd_, firstBinding,
                           static_cast<std::uint32_t>(bufs.size()),
                           bufs.data(), offs.data());
}

void VulkanCommandList::SetIndexBuffer(IRHIBuffer* buffer, std::uint64_t offset, IndexType indexType)
{
    vkCmdBindIndexBuffer(cmd_, static_cast<VulkanBuffer*>(buffer)->GetVkBuffer(),
                         offset, ToVkIndexType(indexType));
}

void VulkanCommandList::BindBindlessHeaps(std::span<IRHIBindlessHeap* const> /*heaps*/) {}
void VulkanCommandList::SetPushConstants(std::uint32_t /*offset*/, std::uint32_t /*size*/,
                                         const void* /*data*/) {}
void VulkanCommandList::SetDescriptorSet(std::uint32_t /*setIndex*/, IRHIDescriptorSet* /*set*/) {}

void VulkanCommandList::Draw(std::uint32_t v, std::uint32_t i, std::uint32_t fv, std::uint32_t fi)
{
    vkCmdDraw(cmd_, v, i, fv, fi);
}

void VulkanCommandList::DrawIndexed(std::uint32_t ic, std::uint32_t inst, std::uint32_t fi,
                                    std::int32_t vo, std::uint32_t fInst)
{
    vkCmdDrawIndexed(cmd_, ic, inst, fi, vo, fInst);
}

void VulkanCommandList::DrawIndirect(IRHIBuffer* args, std::uint64_t offset,
                                     std::uint32_t drawCount, std::uint32_t stride)
{
    vkCmdDrawIndirect(cmd_, static_cast<VulkanBuffer*>(args)->GetVkBuffer(),
                      offset, drawCount, stride);
}

void VulkanCommandList::DrawIndexedIndirect(IRHIBuffer* args, std::uint64_t offset,
                                            std::uint32_t drawCount, std::uint32_t stride)
{
    vkCmdDrawIndexedIndirect(cmd_, static_cast<VulkanBuffer*>(args)->GetVkBuffer(),
                             offset, drawCount, stride);
}

void VulkanCommandList::DrawIndirectCount(IRHIBuffer* args, std::uint64_t argsOff,
                                          IRHIBuffer* cnt, std::uint64_t cntOff,
                                          std::uint32_t maxDraw, std::uint32_t stride)
{
    vkCmdDrawIndirectCount(cmd_,
                           static_cast<VulkanBuffer*>(args)->GetVkBuffer(), argsOff,
                           static_cast<VulkanBuffer*>(cnt)->GetVkBuffer(),  cntOff,
                           maxDraw, stride);
}

void VulkanCommandList::Dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    vkCmdDispatch(cmd_, x, y, z);
}

void VulkanCommandList::DispatchIndirect(IRHIBuffer* args, std::uint64_t offset)
{
    vkCmdDispatchIndirect(cmd_, static_cast<VulkanBuffer*>(args)->GetVkBuffer(), offset);
}

void VulkanCommandList::DispatchMesh(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    if (vkCmdDrawMeshTasksEXT) vkCmdDrawMeshTasksEXT(cmd_, x, y, z);
}
void VulkanCommandList::DispatchMeshIndirect(IRHIBuffer* args, std::uint64_t off,
                                             std::uint32_t draws, std::uint32_t stride)
{
    if (vkCmdDrawMeshTasksIndirectEXT)
        vkCmdDrawMeshTasksIndirectEXT(cmd_,
                                      static_cast<VulkanBuffer*>(args)->GetVkBuffer(),
                                      off, draws, stride);
}

void VulkanCommandList::DispatchRays(const DispatchRaysDesc& /*desc*/)
{
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed))
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "VulkanCommandList::DispatchRays not implemented; call dropped");
    }
}

void VulkanCommandList::BuildAccelerationStructures(
    std::span<const AccelerationStructureBuildDesc> /*builds*/)
{
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed))
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "VulkanCommandList::BuildAccelerationStructures not implemented; call dropped");
    }
}

void VulkanCommandList::CopyBuffer(IRHIBuffer* dst, std::uint64_t dstO,
                                   IRHIBuffer* src, std::uint64_t srcO, std::uint64_t size)
{
    VkBufferCopy region{srcO, dstO, size};
    vkCmdCopyBuffer(cmd_, static_cast<VulkanBuffer*>(src)->GetVkBuffer(),
                          static_cast<VulkanBuffer*>(dst)->GetVkBuffer(), 1, &region);
}

void VulkanCommandList::CopyBufferToTexture(IRHITexture* dst, const TextureRegion& dr,
                                            IRHIBuffer* src, const BufferTextureLayout& sl)
{
    auto* tex = static_cast<VulkanTexture*>(dst);
    VkBufferImageCopy region{};
    region.bufferOffset      = sl.offset;
    region.bufferRowLength   = sl.rowPitch;
    region.bufferImageHeight = sl.slicePitch;
    region.imageSubresource  = ToVkSubresourceLayers(dr.range, tex->GetDesc().format);
    region.imageOffset       = {dr.offset.x, dr.offset.y, dr.offset.z};
    region.imageExtent       = {dr.extent.width, dr.extent.height, dr.extent.depth};
    vkCmdCopyBufferToImage(cmd_, static_cast<VulkanBuffer*>(src)->GetVkBuffer(),
                           tex->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanCommandList::CopyTextureToBuffer(IRHIBuffer* dst, const BufferTextureLayout& dl,
                                            IRHITexture* src, const TextureRegion& sr)
{
    auto* tex = static_cast<VulkanTexture*>(src);
    VkBufferImageCopy region{};
    region.bufferOffset      = dl.offset;
    region.bufferRowLength   = dl.rowPitch;
    region.bufferImageHeight = dl.slicePitch;
    region.imageSubresource  = ToVkSubresourceLayers(sr.range, tex->GetDesc().format);
    region.imageOffset       = {sr.offset.x, sr.offset.y, sr.offset.z};
    region.imageExtent       = {sr.extent.width, sr.extent.height, sr.extent.depth};
    vkCmdCopyImageToBuffer(cmd_, tex->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           static_cast<VulkanBuffer*>(dst)->GetVkBuffer(), 1, &region);
}

void VulkanCommandList::CopyTexture(IRHITexture* dst, const TextureRegion& dr,
                                    IRHITexture* src, const TextureRegion& sr)
{
    auto* d = static_cast<VulkanTexture*>(dst);
    auto* s = static_cast<VulkanTexture*>(src);
    VkImageCopy region{};
    region.srcSubresource = ToVkSubresourceLayers(sr.range, s->GetDesc().format);
    region.srcOffset      = {sr.offset.x, sr.offset.y, sr.offset.z};
    region.dstSubresource = ToVkSubresourceLayers(dr.range, d->GetDesc().format);
    region.dstOffset      = {dr.offset.x, dr.offset.y, dr.offset.z};
    region.extent         = {dr.extent.width, dr.extent.height, dr.extent.depth};
    vkCmdCopyImage(cmd_, s->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         d->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanCommandList::ClearColor(IRHITextureView* view, const ClearColorValue& color)
{
    auto* v = static_cast<VulkanTextureView*>(view);
    auto* t = static_cast<VulkanTexture*>(v->GetTexture());
    VkClearColorValue c{};
    c.float32[0] = color.r; c.float32[1] = color.g;
    c.float32[2] = color.b; c.float32[3] = color.a;
    VkImageSubresourceRange r{};
    r.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    r.baseMipLevel   = v->GetDesc().range.baseMipLevel;
    r.levelCount     = v->GetDesc().range.mipLevelCount;
    r.baseArrayLayer = v->GetDesc().range.baseArrayLayer;
    r.layerCount     = v->GetDesc().range.arrayLayerCount;
    vkCmdClearColorImage(cmd_, t->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &c, 1, &r);
}

void VulkanCommandList::ClearDepthStencil(IRHITextureView* view, float depth, std::uint8_t stencil)
{
    auto* v = static_cast<VulkanTextureView*>(view);
    auto* t = static_cast<VulkanTexture*>(v->GetTexture());
    VkClearDepthStencilValue c{depth, stencil};
    VkImageSubresourceRange r{};
    r.aspectMask     = FormatAspect(t->GetDesc().format);
    r.baseMipLevel   = v->GetDesc().range.baseMipLevel;
    r.levelCount     = v->GetDesc().range.mipLevelCount;
    r.baseArrayLayer = v->GetDesc().range.baseArrayLayer;
    r.layerCount     = v->GetDesc().range.arrayLayerCount;
    vkCmdClearDepthStencilImage(cmd_, t->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                &c, 1, &r);
}

void VulkanCommandList::ResolveTexture(IRHITexture* /*dst*/, IRHITexture* /*src*/,
                                       const ResolveDesc& /*desc*/)
{
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed))
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "VulkanCommandList::ResolveTexture not implemented; call dropped");
    }
}

void VulkanCommandList::BeginQuery(IRHIQueryPool* /*pool*/, std::uint32_t /*index*/) {}
void VulkanCommandList::EndQuery(IRHIQueryPool* /*pool*/, std::uint32_t /*index*/) {}
void VulkanCommandList::WriteTimestamp(IRHIQueryPool* /*pool*/, std::uint32_t /*index*/) {}
void VulkanCommandList::ResolveQueries(IRHIQueryPool* /*pool*/, std::uint32_t /*firstIndex*/,
                                       std::uint32_t /*count*/, IRHIBuffer* /*dst*/,
                                       std::uint64_t /*dstOffset*/) {}

void VulkanCommandList::PushDebugMarker(std::string_view name, std::uint32_t color)
{
    // 1. Native debug-utils label so RenderDoc / Nsight / PIX hooks see it.
    if (device_->HasDebugUtils() && vkCmdBeginDebugUtilsLabelEXT)
    {
        char buf[256];
        const std::size_t n = name.size() < sizeof(buf) - 1 ? name.size() : sizeof(buf) - 1;
        std::memcpy(buf, name.data(), n);
        buf[n] = '\0';
        VkDebugUtilsLabelEXT label{};
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pLabelName = buf;
        DecodeMarkerColor(color, label.color);
        vkCmdBeginDebugUtilsLabelEXT(cmd_, &label);
    }

    // 2. Forward to the active capture tool (Nsight no-ops since it hooks debug-utils itself).
    if (IGraphicsCaptureTool* cap = device_->GetCaptureTool())
    {
        CaptureMarkerColor c{};
        c.r = static_cast<std::uint8_t>((color >> 24) & 0xFF);
        c.g = static_cast<std::uint8_t>((color >> 16) & 0xFF);
        c.b = static_cast<std::uint8_t>((color >>  8) & 0xFF);
        c.a = static_cast<std::uint8_t>((color      ) & 0xFF);
        cap->PushMarker(this, name, c);
    }

    // 3. Forward to the crash reporter so the dump includes the marker timeline.
    if (IGpuCrashReporter* crash = device_->GetCrashReporter())
    {
        crash->PushMarker(this, name);
    }
}

void VulkanCommandList::PopDebugMarker()
{
    if (device_->HasDebugUtils() && vkCmdEndDebugUtilsLabelEXT)
    {
        vkCmdEndDebugUtilsLabelEXT(cmd_);
    }
    if (IGraphicsCaptureTool* cap = device_->GetCaptureTool())
    {
        cap->PopMarker(this);
    }
    if (IGpuCrashReporter* crash = device_->GetCrashReporter())
    {
        crash->PopMarker(this);
    }
}

void VulkanCommandList::InsertDebugMarker(std::string_view name, std::uint32_t color)
{
    if (device_->HasDebugUtils() && vkCmdInsertDebugUtilsLabelEXT)
    {
        char buf[256];
        const std::size_t n = name.size() < sizeof(buf) - 1 ? name.size() : sizeof(buf) - 1;
        std::memcpy(buf, name.data(), n);
        buf[n] = '\0';
        VkDebugUtilsLabelEXT label{};
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pLabelName = buf;
        DecodeMarkerColor(color, label.color);
        vkCmdInsertDebugUtilsLabelEXT(cmd_, &label);
    }
    if (IGraphicsCaptureTool* cap = device_->GetCaptureTool())
    {
        CaptureMarkerColor c{};
        c.r = static_cast<std::uint8_t>((color >> 24) & 0xFF);
        c.g = static_cast<std::uint8_t>((color >> 16) & 0xFF);
        c.b = static_cast<std::uint8_t>((color >>  8) & 0xFF);
        c.a = static_cast<std::uint8_t>((color      ) & 0xFF);
        cap->InsertMarker(this, name, c);
    }
    if (IGpuCrashReporter* crash = device_->GetCrashReporter())
    {
        crash->InsertMarker(this, name);
    }
}

RHINativeHandle VulkanCommandList::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkCommandBuffer,
                           reinterpret_cast<std::uint64_t>(cmd_)};
}

void VulkanCommandList::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER,
                      reinterpret_cast<std::uint64_t>(cmd_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
