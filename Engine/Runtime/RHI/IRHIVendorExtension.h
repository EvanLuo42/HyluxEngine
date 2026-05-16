/// @file
/// @brief Abstract handle to a vendor-provided SDK (NVAPI, AGS, Intel Xe SDK, console SDKs).
///        The native context is returned as a void* to keep vendor headers out of the public
///        RHI surface; consumers cast to the appropriate type in their own translation unit.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Reference-counted handle to a vendor extension SDK adapter. Acquired via
///        IRHIDevice::QueryVendorExtension. Only valid while the adapter is alive.
class IRHIVendorExtension : public IRHIObject
{
public:
    /// @brief Identifies which vendor SDK this object wraps.
    [[nodiscard]] virtual VendorExtensionKind GetKind() const noexcept = 0;

    /// @brief Returns the SDK version reported by the vendor library.
    [[nodiscard]] virtual std::uint32_t GetSdkVersion() const noexcept = 0;

    /// @brief Returns the opaque native context pointer. Consumers cast it to the
    ///        vendor-specific type (e.g. NVAPI's NvU32* device handle) in their own .cpp.
    [[nodiscard]] virtual void* GetNativeContext() const noexcept = 0;
};

} // namespace Hylux::RHI
