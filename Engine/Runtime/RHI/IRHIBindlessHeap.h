/// @file
/// @brief Bindless descriptor heap interface. Two heaps per device: SrvCbvUav and Sampler.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Reference-counted handle to a shader-visible descriptor heap. Resources created
///        with the corresponding bindless flag are slotted into the heap automatically;
///        callers query the slot via the resource's GetBindlessIndex method.
class IRHIBindlessHeap : public IRHIObject
{
public:
    /// @brief Returns which heap kind this object represents.
    [[nodiscard]] virtual BindlessKind GetKind() const noexcept = 0;

    /// @brief Returns the total number of descriptor slots in the heap.
    [[nodiscard]] virtual std::uint32_t GetCapacity() const noexcept = 0;

    /// @brief Returns the number of slots currently occupied. For diagnostics only; the
    ///        value is a snapshot and may be stale immediately after the call.
    [[nodiscard]] virtual std::uint32_t GetUsedCount() const noexcept = 0;

    /// @brief Returns the underlying heap native handle for graphics debugger interop.
    [[nodiscard]] virtual RHINativeHandle GetNativeHeapHandle() const noexcept = 0;
};

} // namespace Hylux::RHI
