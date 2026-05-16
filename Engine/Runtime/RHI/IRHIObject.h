/// @file
/// @brief Root interface for all reference-counted RHI objects. Provides device back-pointer,
///        debug name accessor, and native handle escape hatch.

#pragma once

#include "Core/Memory/RefCounted.h"
#include "RHI/RHIForward.h"
#include "RHI/RHINativeHandle.h"

#include <string>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Root abstract interface inherited by every RHI resource and object. Lifetime is
///        managed via Hylux::Ref<T> using the inherited RefCounted machinery. The native
///        handle accessor returns a tagged backend pointer for interop with external code
///        (graphics capture SDKs, vendor SDKs) without leaking native API headers.
class IRHIObject : public RefCounted
{
public:
    /// @brief Returns the owning device. Always non-null after successful construction.
    [[nodiscard]] virtual IRHIDevice* GetDevice() const noexcept = 0;

    /// @brief Returns the requested backend native handle, or { None, 0 } if unsupported.
    [[nodiscard]] virtual RHINativeHandle GetNativeHandle(
        NativeHandleQuery query = NativeHandleQuery::Primary) const noexcept = 0;

    /// @brief Assigns a debug label visible in graphics debuggers and validation messages.
    virtual void SetDebugName(std::string_view name) = 0;

    /// @brief Returns the previously assigned debug name, or an empty view if none.
    [[nodiscard]] virtual std::string_view GetDebugName() const noexcept = 0;

protected:
    IRHIObject() = default;
};

} // namespace Hylux::RHI
