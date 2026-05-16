/// @file
/// @brief Root RHI instance interface. Returned by CreateRHIInstance.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Reference-counted root object that owns the backend instance (VkInstance /
///        IDXGIFactory / console SDK handle) and any loaded graphics capture tool.
class IRHIInstance : public IRHIObject
{
public:
    /// @brief Returns the backend kind selected at construction.
    [[nodiscard]] virtual DeviceType GetDeviceType() const noexcept = 0;

    /// @brief Returns the number of enumerated adapters.
    [[nodiscard]] virtual std::uint32_t GetAdapterCount() const noexcept = 0;

    /// @brief Returns the adapter at the given index, or nullptr if out of range.
    [[nodiscard]] virtual Ref<IRHIAdapter> GetAdapter(std::uint32_t index) const = 0;

    /// @brief Returns the adapter most appropriate for the requested workload (discrete
    ///        preferred, otherwise first integrated, otherwise first software).
    [[nodiscard]] virtual Ref<IRHIAdapter> GetDefaultAdapter() const = 0;

    /// @brief Returns the active graphics capture tool, or nullptr if none was loaded.
    [[nodiscard]] virtual IGraphicsCaptureTool* GetCaptureTool() const noexcept = 0;
};

} // namespace Hylux::RHI
