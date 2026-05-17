/// @file
/// @brief GPU_CASE harness for RHI tests. Spawns one doctest SUBCASE per backend in a
///        compact spec (Backends::Vulkan, Backends::D3D12, Backends::All), brings the
///        RHI stack up for that backend, and installs a ValidationLogGuard around the
///        test body so any Error/Fatal log emitted during the test (engine-side RHI
///        validation, backend debug layer, validation layer) fails the subcase.

#pragma once

#include "Core/Logging/LogDispatcher.h"
#include "Core/Logging/LogRecord.h"
#include "Core/Logging/Logger.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIAdapter.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIInstance.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIInstanceDesc.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace Hylux::Tests
{

/// @brief Single-backend selector accepted by the GPU_CASE macro.
enum class Backend : std::uint32_t
{
    Vulkan = 0,
    D3D12  = 1,
    Count  = 2,
};

/// @brief Backend set passed to GPU_CASE. Bit positions correspond 1:1 to Backend.
enum class Backends : std::uint32_t
{
    None   = 0,
    Vulkan = 1u << static_cast<std::uint32_t>(Backend::Vulkan),
    D3D12  = 1u << static_cast<std::uint32_t>(Backend::D3D12),
    All    = Vulkan | D3D12,
};

[[nodiscard]] constexpr Backends operator|(Backends a, Backends b) noexcept
{
    return static_cast<Backends>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool Contains(Backends set, Backend b) noexcept
{
    return (static_cast<std::uint32_t>(set) & (1u << static_cast<std::uint32_t>(b))) != 0u;
}

/// @brief Maps a Backend selector to the RHI device-type enum.
[[nodiscard]] constexpr RHI::DeviceType ToDeviceType(Backend backend) noexcept
{
    switch (backend)
    {
        case Backend::Vulkan: return RHI::DeviceType::Vulkan;
        case Backend::D3D12:  return RHI::DeviceType::D3D12;
        default:              return RHI::DeviceType::Null;
    }
}

/// @brief Returns the subcase label printed in doctest output for this backend.
[[nodiscard]] constexpr const char* BackendSubcaseName(Backend backend) noexcept
{
    switch (backend)
    {
        case Backend::Vulkan: return "Backend::Vulkan";
        case Backend::D3D12:  return "Backend::D3D12";
        default:              return "Backend::Unknown";
    }
}

/// @brief Bring-up bundle for a single GPU backend. supported==false means the test body
///        will not run; skipReason is shown via MESSAGE.
struct GpuFixture
{
    Ref<RHI::IRHIInstance> instance{};
    Ref<RHI::IRHIAdapter>  adapter{};
    Ref<RHI::IRHIDevice>   device{};
    Backend                backend{Backend::Vulkan};
    bool                   supported{false};
    const char*            skipReason{"unknown"};
};

/// @brief Attempts to construct an IRHIInstance/Adapter/Device for the given backend.
///        Never throws; on any failure returns a fixture with supported=false and a
///        human-readable skipReason.
[[nodiscard]] GpuFixture TryCreateBackend(Backend backend);

/// @brief Iterable adapter so the GPU_CASE-expanded for-loop walks the active backend set
///        in a fixed order (Vulkan first, then D3D12).
struct BackendIterator
{
    Backends set;

    struct Iter
    {
        Backends      set;
        std::uint32_t i;
        [[nodiscard]] Backend operator*() const noexcept { return static_cast<Backend>(i); }
        Iter& operator++() noexcept
        {
            do { ++i; } while (i < static_cast<std::uint32_t>(Backend::Count) &&
                               !Contains(set, static_cast<Backend>(i)));
            return *this;
        }
        [[nodiscard]] bool operator!=(const Iter& other) const noexcept { return i != other.i; }
    };

    [[nodiscard]] Iter begin() const noexcept
    {
        Iter it{set, 0u};
        if (it.i < static_cast<std::uint32_t>(Backend::Count) &&
            !Contains(set, static_cast<Backend>(it.i)))
        {
            ++it;
        }
        return it;
    }
    [[nodiscard]] Iter end() const noexcept
    {
        return Iter{set, static_cast<std::uint32_t>(Backend::Count)};
    }
};

[[nodiscard]] constexpr BackendIterator ExpandBackends(Backends set) noexcept
{
    return BackendIterator{set};
}

/// @brief Tee dispatcher: captures every Error/Fatal log record while forwarding to the
///        previously active dispatcher so user-visible output is preserved.
class CapturingDispatcher final : public LogDispatcher
{
public:
    explicit CapturingDispatcher(LogDispatcher* forwardTo) noexcept : forward_(forwardTo) {}

    void Dispatch(const LogRecord& record) override
    {
        if (record.level == LogLevel::Error || record.level == LogLevel::Fatal)
        {
            std::lock_guard guard(mutex_);
            captured_.push_back(Entry{
                record.level,
                record.categoryName != nullptr ? record.categoryName : "",
                std::string(record.message)});
        }
        if (forward_ != nullptr)
        {
            forward_->Dispatch(record);
        }
    }

    void Flush() override
    {
        if (forward_ != nullptr)
        {
            forward_->Flush();
        }
    }

    struct Entry
    {
        LogLevel    level;
        std::string category;
        std::string message;
    };

    [[nodiscard]] std::vector<Entry> Drain()
    {
        std::lock_guard guard(mutex_);
        std::vector<Entry> out;
        out.swap(captured_);
        return out;
    }

private:
    LogDispatcher*     forward_{nullptr};
    std::mutex         mutex_;
    std::vector<Entry> captured_;
};

/// @brief RAII guard installed by GPU_CASE around the test body. Swaps in a recording
///        dispatcher so any Error/Fatal log (engine RHI validation, backend debug layer
///        callbacks routed through HYLUX_LOG, etc.) is captured. On destruction it waits
///        for the device to finish any outstanding work, restores the prior dispatcher,
///        and FAIL_CHECKs the active subcase for every captured record.
class ValidationLogGuard
{
public:
    explicit ValidationLogGuard(RHI::IRHIDevice* device = nullptr)
        : device_(device),
          forward_(Logging::GetActiveDispatcher()),
          dispatcher_(forward_)
    {
        Logging::SetActiveDispatcher(&dispatcher_);
    }

    ~ValidationLogGuard()
    {
        if (device_ != nullptr)
        {
            device_->WaitIdle();
        }
        Logging::SetActiveDispatcher(forward_);
        const auto records = dispatcher_.Drain();
        for (const auto& entry : records)
        {
            FAIL_CHECK("[validation] " << entry.category << " "
                                       << (entry.level == LogLevel::Fatal ? "FATAL" : "ERROR")
                                       << ": " << entry.message);
        }
    }

    ValidationLogGuard(const ValidationLogGuard&)            = delete;
    ValidationLogGuard& operator=(const ValidationLogGuard&) = delete;

private:
    RHI::IRHIDevice*    device_;
    LogDispatcher*      forward_;
    CapturingDispatcher dispatcher_;
};

} // namespace Hylux::Tests

// clang-format off
#define HYLUX_GPU_CASE_CAT_(a, b) a##b
#define HYLUX_GPU_CASE_CAT(a, b)  HYLUX_GPU_CASE_CAT_(a, b)
#define HYLUX_GPU_CASE_BODY       HYLUX_GPU_CASE_CAT(HyluxGpuCaseBody_, __LINE__)
// clang-format on

/// @brief Defines a doctest TEST_CASE that auto-generates one SUBCASE per backend in
///        @p backendsArg (a Hylux::Tests::Backends literal — Vulkan, D3D12, or All).
///        Each subcase brings up an IRHIInstance + Adapter + Device for that backend,
///        installs a ValidationLogGuard, and binds the fixture to a `gpu` reference
///        visible to the test body. Backends that fail to initialize are skipped with
///        a MESSAGE and the body is not run.
///
/// @code
/// GPU_CASE("device exposes a graphics queue", Hylux::Tests::Backends::All)
/// {
///     auto queue = gpu.device->GetQueue(RHI::QueueType::Graphics, 0);
///     CHECK(queue);
/// }
/// @endcode
#define GPU_CASE(testName, backendsArg)                                                           \
    static void HYLUX_GPU_CASE_BODY(Hylux::Tests::GpuFixture & gpu);                              \
    TEST_CASE(testName)                                                                           \
    {                                                                                             \
        for (const Hylux::Tests::Backend hyluxBackend__ :                                         \
             Hylux::Tests::ExpandBackends(backendsArg))                                           \
        {                                                                                         \
            SUBCASE(Hylux::Tests::BackendSubcaseName(hyluxBackend__))                             \
            {                                                                                     \
                Hylux::Tests::GpuFixture gpu =                                                    \
                    Hylux::Tests::TryCreateBackend(hyluxBackend__);                               \
                if (!gpu.supported)                                                               \
                {                                                                                 \
                    MESSAGE("skipping " << Hylux::Tests::BackendSubcaseName(hyluxBackend__)       \
                                        << ": " << gpu.skipReason);                               \
                }                                                                                 \
                else                                                                              \
                {                                                                                 \
                    Hylux::Tests::ValidationLogGuard hyluxValidationGuard__(gpu.device.Get());    \
                    HYLUX_GPU_CASE_BODY(gpu);                                                     \
                }                                                                                 \
            }                                                                                     \
        }                                                                                         \
    }                                                                                             \
    static void HYLUX_GPU_CASE_BODY(Hylux::Tests::GpuFixture & gpu)
