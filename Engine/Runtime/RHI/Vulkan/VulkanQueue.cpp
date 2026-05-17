/// @file
/// @brief VulkanQueue implementation.

#include "RHI/Vulkan/VulkanQueue.h"

#include "RHI/Vulkan/VulkanCommandList.h"
#include "RHI/Vulkan/VulkanDebugUtils.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanEnums.h"
#include "RHI/Vulkan/VulkanFence.h"
#include "RHI/Vulkan/VulkanSurface.h"

namespace Hylux::RHI::Vulkan
{

VulkanQueue::VulkanQueue(VulkanDevice* device, QueueType type, std::uint32_t familyIndex,
                         std::uint32_t indexWithinType, VkQueue queue)
    : VulkanObject(device),
      type_(type), familyIndex_(familyIndex),
      indexWithinType_(indexWithinType), queue_(queue) {}

bool VulkanQueue::Submit(std::span<const SubmitDesc> submits)
{
    if (queue_ == VK_NULL_HANDLE) return false;

    std::vector<VkSubmitInfo2>                       submitInfos;
    std::vector<std::vector<VkSemaphoreSubmitInfo>>  waitInfosStorage;
    std::vector<std::vector<VkSemaphoreSubmitInfo>>  signalInfosStorage;
    std::vector<std::vector<VkCommandBufferSubmitInfo>> cmdInfosStorage;
    submitInfos.reserve(submits.size());
    waitInfosStorage.reserve(submits.size());
    signalInfosStorage.reserve(submits.size());
    cmdInfosStorage.reserve(submits.size());

    for (const SubmitDesc& s : submits)
    {
        auto& waitInfos   = waitInfosStorage.emplace_back();
        auto& signalInfos = signalInfosStorage.emplace_back();
        auto& cmdInfos    = cmdInfosStorage.emplace_back();
        waitInfos.reserve(s.waits.size());
        signalInfos.reserve(s.signals.size());
        cmdInfos.reserve(s.commandLists.size());

        for (const auto& w : s.waits)
        {
            VkSemaphoreSubmitInfo si{};
            si.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            si.semaphore = static_cast<VulkanFence*>(w.fence)->GetVkSemaphore();
            si.value     = w.value;
            si.stageMask = ToVkPipelineStages(w.stageMask);
            waitInfos.push_back(si);
        }
        for (const auto& sig : s.signals)
        {
            VkSemaphoreSubmitInfo si{};
            si.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            si.semaphore = static_cast<VulkanFence*>(sig.fence)->GetVkSemaphore();
            si.value     = sig.value;
            si.stageMask = ToVkPipelineStages(sig.stageMask);
            signalInfos.push_back(si);
        }
        for (IRHICommandList* cl : s.commandLists)
        {
            VkCommandBufferSubmitInfo ci{};
            ci.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            ci.commandBuffer = static_cast<VulkanCommandList*>(cl)->GetVkCommandBuffer();
            cmdInfos.push_back(ci);
        }

        VkSubmitInfo2 si{};
        si.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        si.waitSemaphoreInfoCount   = static_cast<std::uint32_t>(waitInfos.size());
        si.pWaitSemaphoreInfos      = waitInfos.data();
        si.signalSemaphoreInfoCount = static_cast<std::uint32_t>(signalInfos.size());
        si.pSignalSemaphoreInfos    = signalInfos.data();
        si.commandBufferInfoCount   = static_cast<std::uint32_t>(cmdInfos.size());
        si.pCommandBufferInfos      = cmdInfos.data();
        submitInfos.push_back(si);
    }

    const VkResult r = vkQueueSubmit2(queue_,
                                      static_cast<std::uint32_t>(submitInfos.size()),
                                      submitInfos.data(),
                                      VK_NULL_HANDLE);
    if (r != VK_SUCCESS)
    {
        HYLUX_LOG(::Hylux::LogRender, Error, "vkQueueSubmit2 failed: {}", VulkanResultToString(r));
        return false;
    }
    return true;
}

bool VulkanQueue::Present(const PresentDesc& presentDesc)
{
    if (queue_ == VK_NULL_HANDLE || presentDesc.swapchain == nullptr) return false;
    auto* sc = static_cast<VulkanSwapchain*>(presentDesc.swapchain);
    VkSwapchainKHR vkSc = sc->GetVkSwapchain();

    std::vector<VkSemaphore>  waitSemas;
    std::vector<std::uint64_t> waitValues;
    for (const auto& w : presentDesc.waits)
    {
        waitSemas.push_back(static_cast<VulkanFence*>(w.fence)->GetVkSemaphore());
        waitValues.push_back(w.value);
    }

    VkTimelineSemaphoreSubmitInfo timeline{};
    timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline.waitSemaphoreValueCount = static_cast<std::uint32_t>(waitValues.size());
    timeline.pWaitSemaphoreValues    = waitValues.data();

    VkPresentInfoKHR info{};
    info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext              = waitValues.empty() ? nullptr : &timeline;
    info.waitSemaphoreCount = static_cast<std::uint32_t>(waitSemas.size());
    info.pWaitSemaphores    = waitSemas.data();
    info.swapchainCount     = 1;
    info.pSwapchains        = &vkSc;
    info.pImageIndices      = &presentDesc.imageIndex;

    const VkResult r = vkQueuePresentKHR(queue_, &info);
    return r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR;
}

bool VulkanQueue::WaitIdle()
{
    return queue_ != VK_NULL_HANDLE && vkQueueWaitIdle(queue_) == VK_SUCCESS;
}

RHINativeHandle VulkanQueue::GetNativeHandle(NativeHandleQuery /*query*/) const noexcept
{
    return RHINativeHandle{RHINativeHandleKind::VkQueue,
                           reinterpret_cast<std::uint64_t>(queue_)};
}

void VulkanQueue::OnDebugNameChanged()
{
    if (device_ && device_->HasDebugUtils())
    {
        SetObjectName(device_->GetVkDevice(), VK_OBJECT_TYPE_QUEUE,
                      reinterpret_cast<std::uint64_t>(queue_), debugName_);
    }
}

} // namespace Hylux::RHI::Vulkan
