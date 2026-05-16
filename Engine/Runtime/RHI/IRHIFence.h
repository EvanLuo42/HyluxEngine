/// @file
/// @brief Timeline-style fence interface unifying Vulkan timeline semaphores and D3D12 fences.

#pragma once

#include "RHI/IRHIObject.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Sentinel for "wait indefinitely" passed to IRHIFence::Wait.
inline constexpr std::uint64_t kFenceWaitInfinite = 0xFFFFFFFFFFFFFFFFull;

/// @brief Monotonic timeline counter shared between the host and the GPU. Submit/Signal
///        operations advance the value; Wait blocks until the counter reaches the target.
class IRHIFence : public IRHIObject
{
public:
    /// @brief Returns the most recently observed completed value. Read is atomic but the
    ///        value may advance immediately after; use only for diagnostics or fast-path
    ///        polling, not for synchronization decisions.
    [[nodiscard]] virtual std::uint64_t GetCompletedValue() const noexcept = 0;

    /// @brief Blocks until the fence reaches at least the target value or the timeout
    ///        elapses. Returns true on success, false on timeout or device-lost.
    virtual bool Wait(std::uint64_t value, std::uint64_t timeoutNanos = kFenceWaitInfinite) = 0;

    /// @brief Host-side signal that advances the timeline to the given value.
    ///        Returns false if the value is not strictly greater than the current value.
    virtual bool Signal(std::uint64_t value) = 0;
};

} // namespace Hylux::RHI
