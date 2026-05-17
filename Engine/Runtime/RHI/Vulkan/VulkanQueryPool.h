/// @file
/// @brief Vulkan implementation of IRHIQueryPool.

#pragma once

#include "RHI/IRHIQueryPool.h"
#include "RHI/Vulkan/VulkanObject.h"

namespace Hylux::RHI::Vulkan
{

class VulkanQueryPool final : public VulkanObject, public IRHIQueryPool
{
public:
    VulkanQueryPool(VulkanDevice* device, QueryType type, std::uint32_t count);
    ~VulkanQueryPool() override;

    [[nodiscard]] bool IsValid() const noexcept { return pool_ != VK_NULL_HANDLE; }
    [[nodiscard]] QueryType     GetType()  const noexcept override { return type_; }
    [[nodiscard]] std::uint32_t GetCount() const noexcept override { return count_; }

    bool GetResults(std::uint32_t firstIndex, std::uint32_t count,
                    std::span<std::uint64_t> outResults, bool wait) override;

    [[nodiscard]] VkQueryPool GetVkQueryPool() const noexcept { return pool_; }

    [[nodiscard]] RHINativeHandle GetNativeHandle(NativeHandleQuery query) const noexcept override;
    [[nodiscard]] IRHIDevice* GetDevice() const noexcept override { return VulkanObject::GetVulkanDeviceAsRhi(); }
    void SetDebugName(std::string_view name) override { VulkanObject::SetDebugNameImpl(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return VulkanObject::GetDebugNameImpl(); }

protected:
    void OnDebugNameChanged() override;

private:
    QueryType     type_;
    std::uint32_t count_;
    VkQueryPool   pool_{VK_NULL_HANDLE};
};

} // namespace Hylux::RHI::Vulkan
