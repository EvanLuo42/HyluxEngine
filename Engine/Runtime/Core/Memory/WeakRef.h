/// @file
/// @brief Non-owning intrusive weak pointer WeakRef<T>. Promotes to Ref<T> via Lock().

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace Hylux
{

/// @brief Non-owning intrusive weak pointer. Holds a weak reference on T's control block
///        so the object's storage remains addressable until all weak references are
///        released; the object body itself may already be destroyed. Promote to a strong
///        Ref<T> via Lock(); the returned Ref is empty if the object has been destroyed.
template<typename T>
class WeakRef
{
    static_assert(std::is_base_of_v<RefCounted, T>,
                  "WeakRef<T>: T must publicly derive from Hylux::RefCounted");

public:
    using ElementType = T;

    constexpr WeakRef() noexcept = default;
    constexpr WeakRef(std::nullptr_t) noexcept {}

    WeakRef(const WeakRef& other) noexcept : pointer_(other.pointer_)
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddWeakRef();
        }
    }

    WeakRef(WeakRef&& other) noexcept : pointer_(other.pointer_)
    {
        other.pointer_ = nullptr;
    }

    template<typename U,
             typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    WeakRef(const WeakRef<U>& other) noexcept : pointer_(other.GetUnsafe())
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddWeakRef();
        }
    }

    template<typename U,
             typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    WeakRef(const Ref<U>& strong) noexcept : pointer_(strong.Get())
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddWeakRef();
        }
    }

    explicit WeakRef(T* pointer) noexcept : pointer_(pointer)
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddWeakRef();
        }
    }

    ~WeakRef()
    {
        if (pointer_ != nullptr)
        {
            pointer_->ReleaseWeakRef();
        }
    }

    WeakRef& operator=(const WeakRef& other) noexcept
    {
        WeakRef tmp(other);
        Swap(tmp);
        return *this;
    }

    WeakRef& operator=(WeakRef&& other) noexcept
    {
        WeakRef tmp(std::move(other));
        Swap(tmp);
        return *this;
    }

    WeakRef& operator=(const Ref<T>& strong) noexcept
    {
        WeakRef tmp(strong);
        Swap(tmp);
        return *this;
    }

    WeakRef& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    /// @brief Attempts to promote to a strong reference. Returns an empty Ref if the
    ///        object has already been destroyed. Safe to call from any thread.
    [[nodiscard]] Ref<T> Lock() const noexcept
    {
        Ref<T> out;
        if (pointer_ != nullptr && pointer_->TryAddRefFromWeak())
        {
            out.Attach(pointer_);
        }
        return out;
    }

    /// @brief Returns true if the object has been destroyed. The answer may be stale by
    ///        the time it returns; use Lock() for any actual access.
    [[nodiscard]] bool Expired() const noexcept
    {
        return pointer_ == nullptr || pointer_->GetStrongCount() == 0;
    }

    void Reset() noexcept
    {
        if (pointer_ != nullptr)
        {
            pointer_->ReleaseWeakRef();
            pointer_ = nullptr;
        }
    }

    void Swap(WeakRef& other) noexcept
    {
        T* tmp = pointer_;
        pointer_ = other.pointer_;
        other.pointer_ = tmp;
    }

    /// @brief Returns the underlying raw pointer without any safety check or refcount
    ///        change. Exposed only for converting-constructor wiring between WeakRef<U>
    ///        and WeakRef<T>; do not dereference the result for object access.
    [[nodiscard]] T* GetUnsafe() const noexcept { return pointer_; }

private:
    T* pointer_{nullptr};
};

template<typename T>
void Swap(WeakRef<T>& a, WeakRef<T>& b) noexcept
{
    a.Swap(b);
}

} // namespace Hylux
