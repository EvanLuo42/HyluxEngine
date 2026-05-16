/// @file
/// @brief Owning intrusive smart pointer Ref<T> for types derived from RefCounted.

#pragma once

#include "Core/Memory/RefCounted.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace Hylux
{

template<typename T> class WeakRef;

/// @brief Owning intrusive pointer. Construction from a raw T* increments the strong
///        count; destruction decrements it. T must publicly derive from Hylux::RefCounted.
///        Mismatched Detach/Attach is the dominant source of intrusive-ref leaks; use
///        those only at FFI boundaries with strict pairing.
template<typename T>
class Ref
{
    static_assert(std::is_base_of_v<RefCounted, T>,
                  "Ref<T>: T must publicly derive from Hylux::RefCounted");

public:
    using ElementType = T;

    constexpr Ref() noexcept = default;
    constexpr Ref(std::nullptr_t) noexcept {}

    /// @brief Wraps an existing raw pointer and AddRefs it. Pass nullptr to construct an empty Ref.
    explicit Ref(T* pointer) noexcept : pointer_(pointer)
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddRef();
        }
    }

    Ref(const Ref& other) noexcept : pointer_(other.pointer_)
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddRef();
        }
    }

    Ref(Ref&& other) noexcept : pointer_(other.pointer_)
    {
        other.pointer_ = nullptr;
    }

    template<typename U,
             typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    Ref(const Ref<U>& other) noexcept : pointer_(other.Get())
    {
        if (pointer_ != nullptr)
        {
            pointer_->AddRef();
        }
    }

    template<typename U,
             typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    Ref(Ref<U>&& other) noexcept : pointer_(other.Detach())
    {
    }

    ~Ref()
    {
        if (pointer_ != nullptr)
        {
            pointer_->Release();
        }
    }

    Ref& operator=(const Ref& other) noexcept
    {
        Ref tmp(other);
        Swap(tmp);
        return *this;
    }

    Ref& operator=(Ref&& other) noexcept
    {
        Ref tmp(std::move(other));
        Swap(tmp);
        return *this;
    }

    Ref& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    [[nodiscard]] T* Get() const noexcept { return pointer_; }
    T& operator*() const noexcept { return *pointer_; }
    T* operator->() const noexcept { return pointer_; }
    explicit operator bool() const noexcept { return pointer_ != nullptr; }

    void Swap(Ref& other) noexcept
    {
        T* tmp = pointer_;
        pointer_ = other.pointer_;
        other.pointer_ = tmp;
    }

    /// @brief Releases the currently held reference and clears the pointer.
    void Reset() noexcept
    {
        if (pointer_ != nullptr)
        {
            pointer_->Release();
            pointer_ = nullptr;
        }
    }

    /// @brief Replaces the held pointer; AddRefs the new one and Releases the old.
    void Reset(T* pointer) noexcept
    {
        Ref tmp(pointer);
        Swap(tmp);
    }

    /// @brief Releases ownership of the held pointer WITHOUT decrementing the refcount.
    ///        The caller becomes responsible for eventually calling Release on the result.
    ///        Use only at FFI boundaries documented as "returns a +1 ref".
    [[nodiscard]] T* Detach() noexcept
    {
        T* result = pointer_;
        pointer_ = nullptr;
        return result;
    }

    /// @brief Takes ownership of a pointer whose refcount is ALREADY incremented for us.
    ///        Does NOT call AddRef. Use only to consume FFI return values documented as
    ///        "returns a +1 ref". Any previously held pointer is Released first.
    void Attach(T* pointer) noexcept
    {
        if (pointer_ != nullptr)
        {
            pointer_->Release();
        }
        pointer_ = pointer;
    }

private:
    T* pointer_{nullptr};
};

template<typename T, typename U>
[[nodiscard]] bool operator==(const Ref<T>& a, const Ref<U>& b) noexcept
{
    return a.Get() == b.Get();
}

template<typename T, typename U>
[[nodiscard]] bool operator!=(const Ref<T>& a, const Ref<U>& b) noexcept
{
    return a.Get() != b.Get();
}

template<typename T>
[[nodiscard]] bool operator==(const Ref<T>& a, std::nullptr_t) noexcept
{
    return a.Get() == nullptr;
}

template<typename T>
[[nodiscard]] bool operator==(std::nullptr_t, const Ref<T>& a) noexcept
{
    return a.Get() == nullptr;
}

template<typename T>
[[nodiscard]] bool operator!=(const Ref<T>& a, std::nullptr_t) noexcept
{
    return a.Get() != nullptr;
}

template<typename T>
[[nodiscard]] bool operator!=(std::nullptr_t, const Ref<T>& a) noexcept
{
    return a.Get() != nullptr;
}

template<typename T>
void Swap(Ref<T>& a, Ref<T>& b) noexcept
{
    a.Swap(b);
}

} // namespace Hylux
