/// @file
/// @brief Per-frame transient upload ring. One persistent-mapped GPU buffer split into
///        `framesInFlight` slots; Allocate bumps a write cursor within the current slot
///        and returns a CPU pointer + GPU buffer + offset + bindless index. Callers stage
///        per-frame uploads (DrawList instance buffers, indirect args, small dynamic
///        constants) through this without lifecycle bookkeeping.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"

#include <cstdint>

namespace Hylux::RHI
{
class IRHIDevice;
}

namespace Hylux::Renderer
{

/// @brief Bump allocator over a persistent-mapped staging buffer. Game-thread vs render-
///        thread ownership: BeginFrame + Allocate are render-thread only; the underlying
///        buffer's persistent mapping is read by the GPU directly so callers do not need
///        a flush step under HOST_COHERENT memory (the Vulkan VMA default for CpuToGpu).
class UploadHeapManager
{
public:
    static constexpr std::uint32_t kInvalidBindlessIndex = 0xFFFFFFFFu;

    /// @brief Sentinel allocation returned on OOM or pre-init.
    struct Allocation
    {
        void*            cpu{nullptr};
        RHI::IRHIBuffer* gpu{nullptr};
        std::uint64_t    offset{0};
        std::uint32_t    bindlessIndex{kInvalidBindlessIndex};

        [[nodiscard]] explicit operator bool() const noexcept { return cpu != nullptr; }
    };

    UploadHeapManager(RHI::IRHIDevice* device, std::uint32_t framesInFlight, std::uint64_t bytesPerFrame);
    ~UploadHeapManager();

    UploadHeapManager(const UploadHeapManager&) = delete;
    UploadHeapManager& operator=(const UploadHeapManager&) = delete;

    /// @brief Bump-allocate a sub-range within the current frame slot. Alignment must be
    ///        a power of two; the returned offset satisfies the supplied alignment within
    ///        the underlying buffer. Returns {} on OOM (caller falls back / skips).
    [[nodiscard]] Allocation Allocate(std::uint64_t size, std::uint64_t alignment) noexcept;

    /// @brief Resets the cursor to the start of `frameSlot`'s sub-range. Caller is
    ///        responsible for ensuring the GPU has retired prior use of this slot —
    ///        FrameFenceTimeline already gates that for us.
    void BeginFrame(std::uint32_t frameSlot) noexcept;

    [[nodiscard]] bool          IsValid() const noexcept;
    [[nodiscard]] std::uint32_t GetBindlessIndex() const noexcept { return bindlessIndex_; }
    [[nodiscard]] std::uint64_t GetBytesPerFrame() const noexcept { return bytesPerFrame_; }
    [[nodiscard]] std::uint32_t GetFramesInFlight() const noexcept { return framesInFlight_; }

private:
    Ref<RHI::IRHIBuffer> buffer_;
    void*                mapped_{nullptr};
    std::uint32_t        bindlessIndex_{kInvalidBindlessIndex};

    std::uint32_t framesInFlight_{0};
    std::uint64_t bytesPerFrame_{0};
    std::uint32_t currentFrameSlot_{0};
    std::uint64_t currentOffset_{0};
};

} // namespace Hylux::Renderer
