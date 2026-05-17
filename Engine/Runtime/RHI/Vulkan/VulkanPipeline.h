/// @file
/// @brief Vulkan implementations of the three pipeline interfaces and the layout.

#pragma once

#include "RHI/IRHIPipeline.h"
#include "RHI/IRHIPipelineLayout.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanPipelineLayout final : public VulkanObject, public IRHIPipelineLayout
{
public:
    VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDesc& desc);
    ~VulkanPipelineLayout() override;

    [[nodiscard]] bool IsValid() const noexcept { return layout_ != VK_NULL_HANDLE; }
    [[nodiscard]] const PipelineLayoutDesc& GetDesc() const noexcept override { return desc_; }

    [[nodiscard]] VkPipelineLayout GetVkPipelineLayout() const noexcept { return layout_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    PipelineLayoutDesc desc_;
    VkPipelineLayout   layout_{VK_NULL_HANDLE};
};

/// @brief Common base for the three Vulkan pipeline classes; stores the VkPipeline + bind point.
class VulkanPipelineBase
{
public:
    [[nodiscard]] VkPipeline           GetVkPipeline() const noexcept { return pipeline_; }
    [[nodiscard]] VkPipelineBindPoint  GetBindPoint()  const noexcept { return bindPoint_; }

protected:
    VkPipeline          pipeline_{VK_NULL_HANDLE};
    VkPipelineBindPoint bindPoint_{VK_PIPELINE_BIND_POINT_GRAPHICS};
};

class VulkanGraphicsPipeline final : public VulkanObject, public IRHIGraphicsPipeline,
                                     public VulkanPipelineBase
{
public:
    VulkanGraphicsPipeline(VulkanDevice* device, const GraphicsPipelineDesc& desc);
    ~VulkanGraphicsPipeline() override;

    [[nodiscard]] bool IsValid() const noexcept { return pipeline_ != VK_NULL_HANDLE; }
    [[nodiscard]] const GraphicsPipelineDesc& GetDesc() const noexcept override { return desc_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    GraphicsPipelineDesc desc_;
};

class VulkanComputePipeline final : public VulkanObject, public IRHIComputePipeline,
                                    public VulkanPipelineBase
{
public:
    VulkanComputePipeline(VulkanDevice* device, const ComputePipelineDesc& desc);
    ~VulkanComputePipeline() override;

    [[nodiscard]] bool IsValid() const noexcept { return pipeline_ != VK_NULL_HANDLE; }
    [[nodiscard]] const ComputePipelineDesc& GetDesc() const noexcept override { return desc_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    ComputePipelineDesc desc_;
};

class VulkanRayTracingPipeline final : public VulkanObject, public IRHIRayTracingPipeline,
                                       public VulkanPipelineBase
{
public:
    VulkanRayTracingPipeline(VulkanDevice* device, const RayTracingPipelineDesc& desc);
    ~VulkanRayTracingPipeline() override;

    [[nodiscard]] bool IsValid() const noexcept { return pipeline_ != VK_NULL_HANDLE; }
    [[nodiscard]] const RayTracingPipelineDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] std::uint32_t GetShaderGroupHandleSize() const noexcept override
    {
        return handleSize_;
    }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    RayTracingPipelineDesc desc_;
    std::uint32_t          handleSize_{0};
};

} // namespace Hylux::RHI::Vulkan
