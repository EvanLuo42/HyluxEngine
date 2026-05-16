/// @file
/// @brief Type-erased value carrier used by reflected method invocation.

#pragma once

#include "Core/Reflection/TypeId.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace Hylux
{

/// @brief Type-erased value carrying its TypeId.
///        Holds either a shared owning copy (Make) or a non-owning pointer (Borrow).
class Any
{
public:
    Any() noexcept = default;
    Any(const Any&) = default;
    Any(Any&&) noexcept = default;
    Any& operator=(const Any&) = default;
    Any& operator=(Any&&) noexcept = default;
    ~Any() = default;

    /// @brief Constructs an Any that owns a heap-allocated copy of value.
    template<typename T> [[nodiscard]] static Any Make(T value)
    {
        using Decayed = std::decay_t<T>;
        auto holder = std::make_shared<Decayed>(std::move(value));
        Any out;
        out.typeId_ = TypeIdOf<Decayed>();
        out.raw_ = holder.get();
        out.storage_ = std::shared_ptr<void>(std::move(holder), out.raw_);
        return out;
    }

    /// @brief Constructs an Any that borrows pointer without taking ownership.
    template<typename T> [[nodiscard]] static Any Borrow(T* pointer) noexcept
    {
        Any out;
        out.typeId_ = TypeIdOf<std::remove_cv_t<T>>();
        out.raw_ = const_cast<void*>(static_cast<const void*>(pointer));
        return out;
    }

    [[nodiscard]] TypeId Type() const noexcept { return typeId_; }
    [[nodiscard]] bool IsEmpty() const noexcept { return typeId_ == TypeId::Invalid; }
    [[nodiscard]] bool IsOwning() const noexcept { return static_cast<bool>(storage_); }
    [[nodiscard]] void* RawPtr() noexcept { return raw_; }
    [[nodiscard]] const void* RawPtr() const noexcept { return raw_; }

    /// @brief Returns a typed pointer when the stored TypeId matches T, otherwise nullptr.
    template<typename T> [[nodiscard]] T* TryGet() noexcept
    {
        if (typeId_ != TypeIdOf<std::remove_cv_t<T>>())
        {
            return nullptr;
        }
        return static_cast<T*>(raw_);
    }

    template<typename T> [[nodiscard]] const T* TryGet() const noexcept
    {
        if (typeId_ != TypeIdOf<std::remove_cv_t<T>>())
        {
            return nullptr;
        }
        return static_cast<const T*>(raw_);
    }

    /// @brief Returns a typed reference; asserts that T matches the stored TypeId.
    template<typename T> [[nodiscard]] T& Get()
    {
        T* typed = TryGet<T>();
        assert(typed != nullptr && "Any::Get<T>() type mismatch");
        return *typed;
    }

    template<typename T> [[nodiscard]] const T& Get() const
    {
        const T* typed = TryGet<T>();
        assert(typed != nullptr && "Any::Get<T>() type mismatch");
        return *typed;
    }

private:
    TypeId typeId_{TypeId::Invalid};
    std::shared_ptr<void> storage_{};
    void* raw_{nullptr};
};

} // namespace Hylux
