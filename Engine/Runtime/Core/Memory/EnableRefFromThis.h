/// @file
/// @brief Mixin enabling `this`-to-Ref<T> upgrade for RefCounted-derived classes.

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <type_traits>

namespace Hylux
{

/// @brief Mixin that adds RefFromThis() to a RefCounted-derived class. Zero extra storage:
///        the strong-from-this upgrade goes through the intrusive control block on the
///        object itself via TryAddRefFromWeak. Returns an empty Ref if called from inside
///        the destructor (strong count has already reached zero).
template<typename T>
class EnableRefFromThis
{
public:
    /// @brief Returns a Ref<T> to this. Empty if the strong count is already zero.
    [[nodiscard]] Ref<T> RefFromThis() noexcept
    {
        static_assert(std::is_base_of_v<RefCounted, T>,
                      "EnableRefFromThis<T>: T must derive from RefCounted");
        T* self = static_cast<T*>(this);
        Ref<T> out;
        if (self->TryAddRefFromWeak())
        {
            out.Attach(self);
        }
        return out;
    }

    /// @brief Returns a Ref<const T> to this. Empty if the strong count is already zero.
    [[nodiscard]] Ref<const T> RefFromThis() const noexcept
    {
        static_assert(std::is_base_of_v<RefCounted, T>,
                      "EnableRefFromThis<T>: T must derive from RefCounted");
        const T* self = static_cast<const T*>(this);
        Ref<const T> out;
        if (self->TryAddRefFromWeak())
        {
            out.Attach(const_cast<T*>(self));
        }
        return out;
    }

protected:
    EnableRefFromThis() noexcept = default;
    ~EnableRefFromThis() = default;
    EnableRefFromThis(const EnableRefFromThis&) noexcept = default;
    EnableRefFromThis(EnableRefFromThis&&) noexcept = default;
    EnableRefFromThis& operator=(const EnableRefFromThis&) noexcept = default;
    EnableRefFromThis& operator=(EnableRefFromThis&&) noexcept = default;
};

} // namespace Hylux
