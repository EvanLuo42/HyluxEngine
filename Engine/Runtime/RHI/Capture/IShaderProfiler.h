/// @file
/// @brief Shader / program-counter sampling abstraction. Mirrors the SASS-level sampler
///        in Nsight Graphics Shader Profiler and the per-shader hotspot reporting in PIX
///        and RGP. Backends without programmatic shader profiling return nullptr from
///        IGraphicsCaptureTool::GetShaderProfiler.

#pragma once

#include "Core/Memory/RefCounted.h"
#include "RHI/RHIForward.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Aggregated sample statistics for a single shader hash, returned by
///        IShaderProfiler::GetShaderStats.
struct ShaderSampleStats
{
    std::uint64_t totalSamples{0};
    std::uint64_t activeSamples{0};
    std::uint64_t stallSamples{0};
    std::uint64_t memoryStallSamples{0};
    std::uint64_t executionUnitStallSamples{0};
};

/// @brief Shader profiler controller. Sampling runs asynchronously between BeginSampling
///        and EndSampling; results are queried by shader hash after the session ends.
class IShaderProfiler : public RefCounted
{
public:
    /// @brief Arms SASS / program-counter sampling. Returns false if a session is already
    ///        active or the backend cannot start sampling.
    virtual bool BeginSampling() = 0;

    /// @brief Stops the active session and finalises results. Safe to call without a
    ///        matching Begin (returns false in that case).
    virtual bool EndSampling() = 0;

    /// @brief Returns true if a sampling session is currently active.
    [[nodiscard]] virtual bool IsSampling() const noexcept = 0;

    /// @brief Returns the number of distinct shader hashes observed in the most recently
    ///        completed sampling session.
    [[nodiscard]] virtual std::uint32_t GetSampledShaderCount() const noexcept = 0;

    /// @brief Returns the shader hash at the given index for the most recently completed
    ///        session, or 0 if the index is out of range.
    [[nodiscard]] virtual std::uint64_t GetSampledShaderHash(std::uint32_t index) const noexcept = 0;

    /// @brief Fills outStats with aggregated samples for the given shader hash. Returns
    ///        false if the hash was not observed in the most recent session.
    [[nodiscard]] virtual bool GetShaderStats(std::uint64_t shaderHash,
                                              ShaderSampleStats& outStats) const = 0;
};

} // namespace Hylux::RHI
