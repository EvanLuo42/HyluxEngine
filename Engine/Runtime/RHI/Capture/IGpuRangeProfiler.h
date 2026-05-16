/// @file
/// @brief Counter / metric sampling abstraction modeled on NVIDIA PerfWorks (Nsight Range
///        Profiler), PIX timing capture counter readback, and Radeon GPU Profiler counter
///        sets. Backends that do not expose programmatic counter sampling return nullptr
///        from IGraphicsCaptureTool::GetRangeProfiler.

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace Hylux::RHI
{

/// @brief Static description of a metric exposed by the backend. Borrowed string views
///        remain valid for the lifetime of the IGpuRangeProfiler.
struct GpuMetricDesc
{
    std::string_view name;
    std::string_view unit;
    std::string_view description;
};

/// @brief Sampled value for a single metric inside a named range.
struct GpuMetricValue
{
    std::string_view name;
    double           value{0.0};
    std::string_view unit;
};

/// @brief Pre-compiled metric configuration. Some backends require compiling the active
///        metric list ahead of time so they can plan the necessary replay passes.
class IGpuMetricSet : public RefCounted
{
public:
    /// @brief Returns the metrics that make up this set, in the order they were requested.
    [[nodiscard]] virtual std::span<const GpuMetricDesc> GetMetrics() const noexcept = 0;

    /// @brief Returns the number of replay passes the backend needs to fully resolve all
    ///        metrics in this set (1 when single-pass).
    [[nodiscard]] virtual std::uint32_t GetRequiredPassCount() const noexcept = 0;
};

/// @brief Range-based GPU counter profiler. Usage:
///         1) Enumerate metrics (GetAvailableMetricCount / GetAvailableMetric).
///         2) Create an IGpuMetricSet from the chosen names.
///         3) For each required replay pass: BeginPass, record commands with BeginRange /
///            EndRange around regions of interest, EndPass.
///         4) Poll ArePassResultsReady, then ReadRangeResults per range.
class IGpuRangeProfiler : public RefCounted
{
public:
    /// @brief Number of metrics exposed by the backend on the current device.
    [[nodiscard]] virtual std::uint32_t GetAvailableMetricCount() const noexcept = 0;

    /// @brief Returns metadata for the metric at the given index, or an empty descriptor
    ///        if the index is out of range.
    [[nodiscard]] virtual GpuMetricDesc GetAvailableMetric(std::uint32_t index) const noexcept = 0;

    /// @brief Compiles a metric set from the given metric names. Returns nullptr if any
    ///        name is unknown to the backend.
    virtual Ref<IGpuMetricSet> CreateMetricSet(std::span<const std::string_view> metrics) = 0;

    /// @brief Begins a profiling pass on the given command list. The same set is replayed
    ///        once per pass reported by IGpuMetricSet::GetRequiredPassCount.
    virtual bool BeginPass(IRHICommandList* commandList, IGpuMetricSet* set,
                           std::uint32_t passIndex) = 0;

    /// @brief Ends the pass that was opened by BeginPass.
    virtual bool EndPass(IRHICommandList* commandList) = 0;

    /// @brief Opens a named range inside the current pass. Counters are reported per range.
    virtual void BeginRange(IRHICommandList* commandList, std::string_view name) = 0;

    /// @brief Closes the most recent open range on this command list.
    virtual void EndRange(IRHICommandList* commandList) = 0;

    /// @brief Returns true once the backend has resolved counter data for every pass
    ///        recorded so far. Pollable after the final submit fences are signaled.
    [[nodiscard]] virtual bool ArePassResultsReady() const noexcept = 0;

    /// @brief Copies the resolved counter values for a named range into the output span.
    ///        Returns the number of values written; 0 if the range is unknown or the
    ///        results are not yet ready.
    virtual std::uint32_t ReadRangeResults(std::string_view rangeName,
                                           std::span<GpuMetricValue> outValues) = 0;
};

} // namespace Hylux::RHI
