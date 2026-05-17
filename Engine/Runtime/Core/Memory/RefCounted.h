/// @file
/// @brief Intrusive atomic reference-counted base class. Holds strong and weak counts
///        in the same allocation as the derived object. Pair with Ref<T> / WeakRef<T> /
///        MakeRef<T>() to manage lifetime.

#pragma once

#include "Core/Math/Detail/PlatformDefines.h"

#include <atomic>
#include <cstdint>
#include <new>

namespace Hylux
{

/// @brief Intrusive atomic refcount base. Two atomic 32-bit counters (strong + weak) are
///        embedded in the object via inheritance. Initial strong count is 0; initial weak
///        count is 1. The constant +1 on weak is a phantom reference collectively held by
///        the set of live strong references and released exactly once when the last strong
///        reference drops.
///
///        Lifetime model:
///         - Strong reaches 0: OnZeroStrongRefs() runs (default: invoke the most-derived
///           destructor in place), then the phantom weak reference is released.
///         - Weak reaches 0: the raw storage is freed via global ::operator delete.
///
///        Allocation contract: instances must be heap-allocated via MakeRef<T>() or
///        new T(...). Custom allocators (pools, arenas) must override OnZeroStrongRefs and
///        currently have no hook for the weak-zero storage-free path; that is a v2 extension.
///
///        Threading: all four primary methods (AddRef, Release, AddWeakRef, ReleaseWeakRef)
///        and TryAddRefFromWeak are safe to call from any thread.
class RefCounted
{
public:
    RefCounted(const RefCounted&) = delete;
    RefCounted(RefCounted&&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;
    RefCounted& operator=(RefCounted&&) = delete;

    /// @brief Increments the strong reference count. The caller must already hold a
    ///        valid pointer to this object (i.e. the count is currently >= 1, or this is
    ///        a freshly constructed object being adopted by its first Ref).
    HYLUX_FORCEINLINE void AddRef() const noexcept
    {
        strongCount_.fetch_add(1, std::memory_order_relaxed);
    }

    /// @brief Decrements the strong reference count. When it reaches zero,
    ///        OnZeroStrongRefs() runs and the phantom weak reference is released.
    ///        Calling Release more times than AddRef is undefined behavior.
    HYLUX_FORCEINLINE void Release() const noexcept
    {
        if (strongCount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            OnZeroStrongRefs();
            ReleaseWeakRef();
        }
    }

    /// @brief Increments the weak reference count. The caller must already hold at least
    ///        one weak reference (or a strong reference, which implies the phantom weak).
    HYLUX_FORCEINLINE void AddWeakRef() const noexcept
    {
        weakCount_.fetch_add(1, std::memory_order_relaxed);
    }

    /// @brief Decrements the weak reference count. When it reaches zero, the raw storage
    ///        of this object is freed via the global ::operator delete.
    HYLUX_FORCEINLINE void ReleaseWeakRef() const noexcept
    {
        if (weakCount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            ::operator delete(const_cast<void*>(dynamic_cast<const void*>(this)));
        }
    }

    /// @brief Atomically promotes a weak reference to a strong reference. The caller must
    ///        already hold a weak reference (so the control block is guaranteed addressable).
    /// @return True if the object is still alive and the strong count was incremented;
    ///         false if the strong count had already reached zero.
    HYLUX_FORCEINLINE bool TryAddRefFromWeak() const noexcept
    {
        std::uint32_t current = strongCount_.load(std::memory_order_relaxed);
        while (current != 0)
        {
            if (strongCount_.compare_exchange_weak(
                    current,
                    current + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    /// @brief Snapshot of the strong reference count. For diagnostics and assertions only.
    [[nodiscard]] std::uint32_t GetStrongCount() const noexcept
    {
        return strongCount_.load(std::memory_order_relaxed);
    }

    /// @brief Snapshot of the weak reference count, including the phantom +1 held by the
    ///        set of live strong references. For diagnostics and assertions only.
    [[nodiscard]] std::uint32_t GetWeakCount() const noexcept
    {
        return weakCount_.load(std::memory_order_relaxed);
    }

protected:
    RefCounted() noexcept : strongCount_(0), weakCount_(1) {}

    virtual ~RefCounted() = default;

    /// @brief Invoked when the strong reference count reaches zero. The default
    ///        implementation destroys the object in place via virtual dispatch on the
    ///        destructor; the storage is freed later when the weak count reaches zero.
    ///        Override to redirect to a pool or other custom recycling strategy; any
    ///        override must arrange for the object's destructor to run.
    virtual void OnZeroStrongRefs() const noexcept
    {
        const_cast<RefCounted*>(this)->~RefCounted();
    }

private:
    mutable std::atomic<std::uint32_t> strongCount_;
    mutable std::atomic<std::uint32_t> weakCount_;
};

} // namespace Hylux
