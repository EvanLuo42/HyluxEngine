/// @file
/// @brief Descriptor structs for device creation, queue request, and reported device limits.

#pragma once

#include "RHI/RHIEnums.h"
#include "RHI/RHIFeature.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIValidation.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Per-bindless-heap capacity request, applied on device creation.
struct BindlessHeapSizes
{
    std::uint32_t srvCbvUavCapacity{500000};
    std::uint32_t samplerCapacity{2048};
};

/// @brief Description supplied to IRHIAdapter::CreateDevice.
struct DeviceDesc
{
    FeatureSet           enabledFeatures{};
    std::uint32_t        graphicsQueueCount{1};
    std::uint32_t        computeQueueCount{1};
    std::uint32_t        copyQueueCount{1};
    std::uint32_t        videoEncodeQueueCount{0};
    std::uint32_t        videoDecodeQueueCount{0};
    BindlessHeapSizes    bindlessSizes{};
    RhiValidationLevel   rhiValidation{RhiValidationLevel::Off};
    GapiValidationLevel  gapiValidation{GapiValidationLevel::Off};
};

/// @brief Read-only device limit set reported by IRHIDevice::GetLimits.
struct DeviceLimits
{
    std::uint64_t maxBufferSize{0};
    std::uint32_t maxTexture1DSize{0};
    std::uint32_t maxTexture2DSize{0};
    std::uint32_t maxTexture3DSize{0};
    std::uint32_t maxTextureCubeSize{0};
    std::uint32_t maxTextureArrayLayers{0};
    std::uint32_t maxColorAttachments{0};
    std::uint32_t maxVertexAttributes{0};
    std::uint32_t maxComputeWorkgroupCountX{0};
    std::uint32_t maxComputeWorkgroupCountY{0};
    std::uint32_t maxComputeWorkgroupCountZ{0};
    std::uint32_t maxComputeWorkgroupSizeX{0};
    std::uint32_t maxComputeWorkgroupSizeY{0};
    std::uint32_t maxComputeWorkgroupSizeZ{0};
    std::uint32_t maxComputeWorkgroupInvocations{0};
    std::uint32_t maxPushConstantSize{0};
    std::uint32_t minUniformBufferOffsetAlignment{0};
    std::uint32_t minStorageBufferOffsetAlignment{0};
    std::uint32_t minTexelBufferOffsetAlignment{0};
    std::uint32_t timestampPeriodNanos{0};
    std::uint32_t maxBindlessSrvCbvUav{0};
    std::uint32_t maxBindlessSampler{0};
    std::uint32_t maxRayRecursionDepth{0};
};

} // namespace Hylux::RHI
