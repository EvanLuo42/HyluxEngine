/// @file
/// @brief Central device interface. Creates every kind of RHI resource and pipeline,
///        exposes feature/limit queries, and provides access to bindless heaps and queues.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIObject.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIForward.h"
#include "RHI/IRHISurface.h"
#include "RHI/RHIPipelineDesc.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHIValidation.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Logical GPU device. Owns queues, descriptor heaps, and the lifetime root for all
///        resources created through it. Construction is performed via IRHIAdapter::CreateDevice.
class IRHIDevice : public IRHIObject
{
public:
    /// @brief Returns the descriptor the device was created with.
    [[nodiscard]] virtual const DeviceDesc& GetDesc() const noexcept = 0;

    /// @brief Returns which underlying graphics API backs the device.
    [[nodiscard]] virtual DeviceType GetDeviceType() const noexcept = 0;

    /// @brief Returns the adapter the device was created on.
    [[nodiscard]] virtual IRHIAdapter* GetAdapter() const noexcept = 0;

    /// @brief Returns the owning instance.
    [[nodiscard]] virtual IRHIInstance* GetInstance() const noexcept = 0;

    /// @brief Returns the set of features that were actually enabled at creation.
    [[nodiscard]] virtual FeatureSet GetEnabledFeatures() const noexcept = 0;

    /// @brief Convenience accessor: returns true if the feature is in GetEnabledFeatures().
    [[nodiscard]] virtual bool IsFeatureEnabled(Feature feature) const noexcept = 0;

    /// @brief Returns concrete device limits reported by the backend.
    [[nodiscard]] virtual const DeviceLimits& GetLimits() const noexcept = 0;

    /// @brief Returns supported usages for a texel format on this device.
    [[nodiscard]] virtual FormatSupport GetFormatSupport(Format format) const noexcept = 0;

    [[nodiscard]] virtual Ref<IRHIQueue> GetQueue(QueueType type,
                                                  std::uint32_t index = 0) const = 0;

    [[nodiscard]] virtual std::uint32_t GetQueueCount(QueueType type) const noexcept = 0;

    virtual Ref<IRHIBuffer>      CreateBuffer(const BufferDesc& desc) = 0;
    virtual Ref<IRHITexture>     CreateTexture(const TextureDesc& desc) = 0;
    virtual Ref<IRHITextureView> CreateTextureView(IRHITexture* texture,
                                                   const TextureViewDesc& desc) = 0;
    virtual Ref<IRHISampler>     CreateSampler(const SamplerDesc& desc) = 0;
    virtual Ref<IRHIShaderModule> CreateShaderModule(const ShaderBytecode& bytecode) = 0;

    virtual Ref<IRHIPipelineLayout>     CreatePipelineLayout(const PipelineLayoutDesc& desc) = 0;
    virtual Ref<IRHIGraphicsPipeline>   CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual Ref<IRHIComputePipeline>    CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual Ref<IRHIRayTracingPipeline> CreateRayTracingPipeline(
        const RayTracingPipelineDesc& desc) = 0;

    virtual Ref<IRHIAccelerationStructure> CreateBlas(const BlasDesc& desc) = 0;
    virtual Ref<IRHIAccelerationStructure> CreateTlas(const TlasDesc& desc) = 0;

    /// @brief Returns the bindless descriptor heap of the given kind. Heaps are created
    ///        once at device init using DeviceDesc::bindlessSizes; this is a non-owning
    ///        accessor that just bumps the refcount.
    [[nodiscard]] virtual Ref<IRHIBindlessHeap> GetBindlessHeap(BindlessKind kind) = 0;

    virtual Ref<IRHIFence>       CreateFence(std::uint64_t initialValue = 0) = 0;
    virtual Ref<IRHICommandPool> CreateCommandPool(QueueType type,
                                                   CommandPoolFlags flags = {}) = 0;
    virtual Ref<IRHIQueryPool>   CreateQueryPool(QueryType type, std::uint32_t count) = 0;

    virtual Ref<IRHISurface>   CreateSurface(const PlatformWindowHandle& window) = 0;
    virtual Ref<IRHISwapchain> CreateSwapchain(IRHISurface* surface,
                                               const SwapchainDesc& desc) = 0;

    virtual Ref<IRHIDescriptorSetLayout> CreateDescriptorSetLayout(
        const DescriptorSetLayoutDesc& desc) = 0;
    virtual Ref<IRHIDescriptorSet> AllocateDescriptorSet(IRHIDescriptorSetLayout* layout) = 0;

    /// @brief Returns the vendor extension interface for the requested SDK, or nullptr if
    ///        the SDK is unavailable on this device.
    virtual Ref<IRHIVendorExtension> QueryVendorExtension(VendorExtensionKind kind) = 0;

    /// @brief Returns the active capture tool, or nullptr if no SDK was loaded.
    [[nodiscard]] virtual IGraphicsCaptureTool* GetCaptureTool() const noexcept = 0;

    /// @brief Blocks until all submitted work on every queue has completed.
    virtual bool WaitIdle() = 0;
};

} // namespace Hylux::RHI
