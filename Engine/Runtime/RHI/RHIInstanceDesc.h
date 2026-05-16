/// @file
/// @brief Descriptor structs for instance and adapter creation.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIValidation.h"

#include <cstdint>
#include <string>

namespace Hylux::RHI
{

/// @brief Description supplied to CreateRHIInstance. Drives backend selection, validation
///        layer configuration, and capture tool loading. Must outlive the call.
struct InstanceDesc
{
    DeviceType          preferredDevice{DeviceType::Vulkan};
    RhiValidationLevel  rhiValidation{RhiValidationLevel::Off};
    GapiValidationLevel gapiValidation{GapiValidationLevel::Off};
    CaptureToolKind     captureTool{CaptureToolKind::Auto};
    std::string         applicationName{};
    std::uint32_t       applicationVersion{0};
    std::string         engineName{"HyluxEngine"};
    std::uint32_t       engineVersion{0};
    FeatureSet          requiredFeatures{};
    FeatureSet          desiredFeatures{};
};

/// @brief Read-only adapter description returned by IRHIAdapter::GetDesc.
struct AdapterDesc
{
    std::string   name{};
    std::uint32_t vendorId{0};
    std::uint32_t deviceId{0};
    std::uint64_t dedicatedVideoMemory{0};
    std::uint64_t systemMemory{0};
    DeviceType    type{DeviceType::Null};
    AdapterFlags  flags{AdapterFlags::None};
};

} // namespace Hylux::RHI
