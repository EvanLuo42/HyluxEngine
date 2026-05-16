/// @file
/// @brief Factory MakeRef<T>(args...) that heap-allocates a RefCounted-derived object
///        and returns the sole strong Ref<T> owning it.

#pragma once

#include "Core/Memory/Ref.h"

#include <utility>

namespace Hylux
{

/// @brief Constructs a new T(args...) on the heap and returns a Ref<T> holding the sole
///        strong reference. T must publicly derive from RefCounted. The returned Ref is
///        the only owner; copying it shares ownership in the standard way.
template<typename T, typename... Args>
[[nodiscard]] Ref<T> MakeRef(Args&&... args)
{
    T* raw = new T(std::forward<Args>(args)...);
    raw->AddRef();
    Ref<T> result;
    result.Attach(raw);
    return result;
}

} // namespace Hylux
