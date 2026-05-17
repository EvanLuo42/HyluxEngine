/// @file
/// @brief Vulkan implementation of IRHIDevice. Owns VkDevice, VmaAllocator, per-queue
///        VulkanQueue objects, and (optionally) the loaded GPU crash reporter.

#pragma once

#include "RHI/Capture/IGraphicsCaptureTool.h"
#include "RHI/Diagnostics/IGpuCrashReporter.h"
#include "RHI/IRHIDevice.h"
#include "RHI/Vulkan/VulkanBindlessHeap.h"
#include "RHI/Vulkan/VulkanCommon.h"

#include <vk_mem_alloc.h>

#include <array>
#include <vector>

namespace Hylux::RHI::Vulkan
{

class VulkanQueue;

class VulkanDevice final : public IRHIDevice
{
public:
    VulkanDevice(VulkanAdapter* adapter, const DeviceDesc& desc);
    ~VulkanDevice() override;

    [[nodiscard]] bool IsValid() const noexcept { return device_ != VK_NULL_HANDLE; }

    // IRHIDevice ---------------------------------------------------------------------------
    [[nodiscard]] const DeviceDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] DeviceType        GetDeviceType() const noexcept override { return DeviceType::Vulkan; }
    [[nodiscard]] IRHIAdapter*      GetAdapter() const noexcept override;
    [[nodiscard]] IRHIInstance*     GetInstance() const noexcept override;
    [[nodiscard]] FeatureSet        GetEnabledFeatures() const noexcept override { return enabledFeatures_; }
    [[nodiscard]] bool              IsFeatureEnabled(Feature feature) const noexcept override
    {
        return enabledFeatures_.Has(feature);
    }
    [[nodiscard]] const DeviceLimits& GetLimits() const noexcept override { return limits_; }
    [[nodiscard]] FormatSupport       GetFormatSupport(Format format) const noexcept override;

    [[nodiscard]] Ref<IRHIQueue>      GetQueue(QueueType type, std::uint32_t index = 0) const override;
    [[nodiscard]] std::uint32_t       GetQueueCount(QueueType type) const noexcept override;

    Ref<IRHIBuffer>       CreateBuffer(const BufferDesc& desc) override;
    Ref<IRHITexture>      CreateTexture(const TextureDesc& desc) override;
    Ref<IRHITextureView>  CreateTextureView(IRHITexture* texture, const TextureViewDesc& desc) override;
    Ref<IRHISampler>      CreateSampler(const SamplerDesc& desc) override;
    Ref<IRHIShaderModule> CreateShaderModule(const ShaderBytecode& bytecode) override;

    Ref<IRHIPipelineLayout>     CreatePipelineLayout(const PipelineLayoutDesc& desc) override;
    Ref<IRHIGraphicsPipeline>   CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    Ref<IRHIComputePipeline>    CreateComputePipeline(const ComputePipelineDesc& desc) override;
    Ref<IRHIRayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;

    Ref<IRHIAccelerationStructure> CreateBlas(const BlasDesc& desc) override;
    Ref<IRHIAccelerationStructure> CreateTlas(const TlasDesc& desc) override;

    [[nodiscard]] Ref<IRHIBindlessHeap> GetBindlessHeap(BindlessKind kind) override;

    Ref<IRHIFence>       CreateFence(std::uint64_t initialValue = 0) override;
    Ref<IRHICommandPool> CreateCommandPool(QueueType type, CommandPoolFlags flags = {}) override;
    Ref<IRHIQueryPool>   CreateQueryPool(QueryType type, std::uint32_t count) override;

    Ref<IRHISurface>   CreateSurface(const PlatformWindowHandle& window) override;
    Ref<IRHISwapchain> CreateSwapchain(IRHISurface* surface, const SwapchainDesc& desc) override;

    Ref<IRHIDescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) override;
    Ref<IRHIDescriptorSet>       AllocateDescriptorSet(IRHIDescriptorSetLayout* layout) override;

    Ref<IRHIVendorExtension> QueryVendorExtension(VendorExtensionKind kind) override;

    [[nodiscard]] IGraphicsCaptureTool* GetCaptureTool() const noexcept override;
    [[nodiscard]] IGpuCrashReporter*    GetCrashReporter() const noexcept override;

    bool WaitIdle() override;

    // VulkanDevice native accessors -------------------------------------------------------
    [[nodiscard]] VkDevice         GetVkDevice() const noexcept       { return device_; }
    [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const noexcept;
    [[nodiscard]] VkInstance       GetVkInstance() const noexcept;
    [[nodiscard]] VmaAllocator     GetVmaAllocator() const noexcept   { return vmaAllocator_; }
    [[nodiscard]] VulkanAdapter*   GetVulkanAdapter() const noexcept  { return adapter_; }
    [[nodiscard]] bool             HasDebugUtils() const noexcept     { return hasDebugUtils_; }

    /// @brief Stores a strong reference to the crash reporter built during adapter
    ///        CreateDevice(); called once during construction by VulkanAdapter.
    void AttachCrashReporter(Ref<IGpuCrashReporter> reporter) noexcept;

    // IRHIObject
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override
    {
        // A device's "owning device" is itself — useful so any IRHIObject* held by engine
        // code (including the device) can be asked for its device without special-casing.
        return const_cast<VulkanDevice*>(this);
    }
    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    void SetDebugName(std::string_view name) override { debugName_.assign(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return debugName_; }

private:
    bool Initialize();
    void Shutdown() noexcept;

    VulkanAdapter*       adapter_{nullptr};
    DeviceDesc           desc_;
    VkDevice             device_{VK_NULL_HANDLE};
    VmaAllocator         vmaAllocator_{nullptr};
    FeatureSet           enabledFeatures_{};
    DeviceLimits         limits_{};
    bool                 hasDebugUtils_{false};

    std::array<std::vector<Ref<VulkanQueue>>, static_cast<std::size_t>(QueueType::Count)> queues_;

    std::array<Ref<VulkanBindlessHeap>, static_cast<std::size_t>(BindlessKind::Count)> bindlessHeaps_;

    Ref<IGpuCrashReporter> crashReporter_;
    std::string            debugName_;
};

} // namespace Hylux::RHI::Vulkan
