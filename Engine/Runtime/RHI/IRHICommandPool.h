/// @file
/// @brief Command pool interface. Allocates command lists bound to a single queue type.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

namespace Hylux::RHI
{

/// @brief Pool of command list allocations tied to a single queue family. Not thread-safe;
///        each recording thread should own its own pool.
class IRHICommandPool : public IRHIObject
{
public:
    /// @brief Returns the queue type this pool services.
    [[nodiscard]] virtual QueueType GetQueueType() const noexcept = 0;

    /// @brief Allocates a new command list ready for Begin().
    virtual Ref<IRHICommandList> AllocateCommandList() = 0;

    /// @brief Resets all command lists allocated from this pool back to the initial state.
    virtual bool Reset() = 0;
};

} // namespace Hylux::RHI
