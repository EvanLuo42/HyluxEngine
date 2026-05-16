/// @file
/// @brief Queue submission, fence wait/signal, and present interfaces.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <span>

namespace Hylux::RHI
{

/// @brief Single fence wait entry within a SubmitDesc.
struct FenceWaitDesc
{
    IRHIFence*        fence{nullptr};
    std::uint64_t     value{0};
    PipelineStageMask stageMask{PipelineStageMask::AllCommands};
};

/// @brief Single fence signal entry within a SubmitDesc.
struct FenceSignalDesc
{
    IRHIFence*        fence{nullptr};
    std::uint64_t     value{0};
    PipelineStageMask stageMask{PipelineStageMask::AllCommands};
};

/// @brief One batch passed to IRHIQueue::Submit.
struct SubmitDesc
{
    std::span<IRHICommandList* const> commandLists{};
    std::span<const FenceWaitDesc>    waits{};
    std::span<const FenceSignalDesc>  signals{};
};

/// @brief Description for IRHIQueue::Present.
struct PresentDesc
{
    IRHISwapchain*                 swapchain{nullptr};
    std::uint32_t                  imageIndex{0};
    std::span<const FenceWaitDesc> waits{};
};

/// @brief Reference-counted handle to a logical queue exposed by IRHIDevice.
class IRHIQueue : public IRHIObject
{
public:
    /// @brief Returns the queue's logical kind.
    [[nodiscard]] virtual QueueType GetType() const noexcept = 0;

    /// @brief Returns the queue's index within its kind.
    [[nodiscard]] virtual std::uint32_t GetIndex() const noexcept = 0;

    /// @brief Submits the listed command list batches to the queue. Returns false on
    ///        device-lost or invalid submission.
    virtual bool Submit(std::span<const SubmitDesc> submits) = 0;

    /// @brief Presents the given swapchain image after waiting on the provided fences.
    virtual bool Present(const PresentDesc& presentDesc) = 0;

    /// @brief Blocks until all previously submitted work on the queue has completed.
    virtual bool WaitIdle() = 0;
};

} // namespace Hylux::RHI
