/// @file
/// @brief Timeline-based GPU trace abstraction. Mirrors the Nsight Graphics "GPU Trace"
///        activity, PIX timing capture, and RGP profile capture — all of which sample
///        hardware unit timing plus a counter timeline over a wall-clock window rather
///        than serializing a frame for replay.

#pragma once

#include "Core/Memory/RefCounted.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <string>

namespace Hylux::RHI
{

/// @brief Configuration for IGpuTraceCapture::BeginTrace. Fields the backend does not
///        understand are silently ignored.
struct GpuTraceDesc
{
    std::string   outputPath{};
    std::uint32_t durationMilliseconds{1000};
    bool          captureUnitLevelTiming{true};
    bool          captureCounterTimeline{true};
    bool          captureMemoryActivity{false};
    bool          openInUiOnComplete{false};
};

/// @brief Timed GPU trace controller. Trace lifetime is asynchronous: BeginTrace arms the
///        capture, EndTrace stops and serialises it to disk. Either call may be a no-op if
///        the loaded SDK does not support timed traces.
class IGpuTraceCapture : public RefCounted
{
public:
    /// @brief Arms a trace. Backends that use a wall-clock window honour
    ///        durationMilliseconds; frame-stepped backends ignore it and trace until
    ///        EndTrace. Returns false on failure or if a trace is already in flight.
    virtual bool BeginTrace(const GpuTraceDesc& desc) = 0;

    /// @brief Stops the in-flight trace and serialises it to the configured output path.
    ///        Safe to call with no trace active (returns false).
    virtual bool EndTrace() = 0;

    /// @brief Returns true if a trace is currently being recorded.
    [[nodiscard]] virtual bool IsTracing() const noexcept = 0;
};

} // namespace Hylux::RHI
