/// @file
/// @brief Physical adapter interface. Enumerated by IRHIInstance and used to create devices.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIObject.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIInstanceDesc.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Reference-counted handle to a physical adapter (Vulkan VkPhysicalDevice /
///        D3D12 IDXGIAdapter / console GPU descriptor).
class IRHIAdapter : public IRHIObject
{
public:
    /// @brief Returns the cached descriptor.
    [[nodiscard]] virtual const AdapterDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the full set of features the adapter supports.
    [[nodiscard]] virtual FeatureSet GetSupportedFeatures() const noexcept = 0;

    /// @brief Returns true if the feature is in GetSupportedFeatures().
    [[nodiscard]] virtual bool IsFeatureSupported(Feature feature) const noexcept = 0;

    /// @brief Creates a device on this adapter with the requested configuration. Returns
    ///        nullptr if a required feature is missing or queue counts are unsupported.
    virtual Ref<IRHIDevice> CreateDevice(const DeviceDesc& desc) = 0;
};

} // namespace Hylux::RHI
