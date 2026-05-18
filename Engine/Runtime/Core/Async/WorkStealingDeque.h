/// @file
/// @brief Chase-Lev work-stealing deque, fixed capacity, pointer-sized slots.
///        Owner thread alone calls PushBottom / PopBottom; any number of thief threads
///        may call Steal concurrently. The deque stores pointers; the caller owns the
///        lifetime of whatever the pointers refer to (typically a heap-allocated task
///        node). Pointer-only slots sidestep the well-known race on the empty-slot edge
///        case that affects naive direct-value Chase-Lev implementations in C++.

#pragma once

#include "Core/Utils/Assert.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace Hylux
{

/// @brief Lock-free deque following Chase & Lev (2005). Single owner (push/pop bottom),
///        multiple thieves (steal from top). Capacity is fixed and must be a power of two;
///        PushBottom returns false if the deque is full rather than growing.
template<typename T>
class WorkStealingDeque
{
    static_assert(std::is_pointer_v<T>, "WorkStealingDeque only stores pointer-sized items");

public:
    /// @brief @p capacity must be a power of two and at least 2.
    explicit WorkStealingDeque(std::size_t capacity)
        : mask_(capacity - 1),
          buffer_(std::make_unique<std::atomic<T>[]>(capacity))
    {
        HYLUX_ASSERT_MSG(capacity >= 2 && (capacity & (capacity - 1)) == 0,
                         "WorkStealingDeque capacity must be a power of two >= 2");
        for (std::size_t i = 0; i < capacity; ++i)
        {
            buffer_[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    WorkStealingDeque(const WorkStealingDeque&)            = delete;
    WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;

    /// @brief Owner-only. Pushes @p item at the bottom. Returns false if the deque is full.
    bool PushBottom(T item) noexcept
    {
        const std::int64_t b = bottom_.load(std::memory_order_relaxed);
        const std::int64_t t = top_.load(std::memory_order_acquire);
        if (b - t >= static_cast<std::int64_t>(mask_ + 1))
        {
            return false;
        }
        buffer_[static_cast<std::size_t>(b) & mask_].store(item, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(b + 1, std::memory_order_relaxed);
        return true;
    }

    /// @brief Owner-only. Pops the bottom item; returns nullptr if the deque is empty
    ///        (including the empty-after-steal race resolution).
    T PopBottom() noexcept
    {
        const std::int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
        bottom_.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::int64_t t = top_.load(std::memory_order_relaxed);

        if (t > b)
        {
            bottom_.store(b + 1, std::memory_order_relaxed);
            return nullptr;
        }

        T item = buffer_[static_cast<std::size_t>(b) & mask_].load(std::memory_order_relaxed);
        if (t < b)
        {
            return item;
        }

        if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst,
                                          std::memory_order_relaxed))
        {
            item = nullptr;
        }
        bottom_.store(b + 1, std::memory_order_relaxed);
        return item;
    }

    /// @brief Thief entry. Returns nullptr if empty or if another thief beat us to the slot;
    ///        callers retry. Distinct return values from concurrent Steals are guaranteed.
    T Steal() noexcept
    {
        std::int64_t t = top_.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const std::int64_t b = bottom_.load(std::memory_order_acquire);

        if (t >= b)
        {
            return nullptr;
        }

        T item = buffer_[static_cast<std::size_t>(t) & mask_].load(std::memory_order_relaxed);
        if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst,
                                          std::memory_order_relaxed))
        {
            return nullptr;
        }
        return item;
    }

    /// @brief Approximate item count, racy by construction. Useful for steal-target heuristics
    ///        and stats; never base correctness on this value.
    [[nodiscard]] std::size_t SizeApprox() const noexcept
    {
        const std::int64_t b = bottom_.load(std::memory_order_relaxed);
        const std::int64_t t = top_.load(std::memory_order_relaxed);
        return b > t ? static_cast<std::size_t>(b - t) : 0;
    }

    /// @brief Fixed capacity of the deque (power of two).
    [[nodiscard]] std::size_t Capacity() const noexcept { return mask_ + 1; }

private:
    const std::size_t                  mask_;
    std::unique_ptr<std::atomic<T>[]>  buffer_;
    std::atomic<std::int64_t>          top_{0};
    std::atomic<std::int64_t>          bottom_{0};
};

} // namespace Hylux
