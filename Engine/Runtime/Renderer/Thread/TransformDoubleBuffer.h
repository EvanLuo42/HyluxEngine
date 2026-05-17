/// @file
/// @brief Game ↔ Render hot-data channel for per-frame transforms. Two GPU storage buffers
///        ("halves") sit persistently mapped; game thread writes into the current writing
///        half and atomically publishes it; the render thread queries the published half's
///        bindless index and lets the GPU read it directly. No transform bytes ever cross
///        the CPU command queue.

#pragma once

#include "Core/Math/Mat3x4.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "Renderer/Proxy/ProxyId.h"

#include <atomic>
#include <cstdint>

namespace Hylux::RHI
{
class IRHIDevice;
}

namespace Hylux::Renderer
{

/// @brief Row-major 3x4 world matrix uploaded into the GPU transform buffer. Same memory
///        layout as the shader-side `float3x4`; `StoreRowMajor` copies the 48 bytes
///        directly into a TransformGpuRecord slot.
using TransformMat3x4 = Math::Mat3x4;

/// @brief GPU-side per-slot record. Layout is locked to 64 bytes so shaders can index the
///        SSBO without strided arithmetic.
struct alignas(16) TransformGpuRecord
{
    float         worldMatrix[12]{};
    std::uint32_t flags{0};
    std::uint32_t pad[3]{};
};
static_assert(sizeof(TransformGpuRecord) == 64, "TransformGpuRecord layout must stay 64 bytes");

/// @brief Double-buffered transform storage. AllocateSlot is callable from any game thread
///        (monotonic atomic counter); WriteTransform / PublishAndFlip are restricted to the
///        single owning game thread per the engine threading contract.
class TransformDoubleBuffer
{
public:
    static constexpr std::uint32_t kInvalidHalfIndex = 0xFFFFFFFFu;

    TransformDoubleBuffer(RHI::IRHIDevice* device, std::uint32_t capacity);
    ~TransformDoubleBuffer();

    TransformDoubleBuffer(const TransformDoubleBuffer&) = delete;
    TransformDoubleBuffer& operator=(const TransformDoubleBuffer&) = delete;

    /// @brief Reserves a slot for a new primitive. Returns the slot index (also encoded as
    ///        the ProxyId). Returns ProxyId::Invalid when capacity is exhausted.
    [[nodiscard]] ProxyId AllocateSlot();

    /// @brief Releases a slot. Stage 4a: a no-op — slots are leaked. A proper free list
    ///        with deferred retirement lands once stages 5+ stress the budget.
    void ReleaseSlot(ProxyId id) noexcept;

    /// @brief Writes the supplied transform into the current writing half. Lock-free; the
    ///        slot is fixed at allocation time. Callers may invoke from multiple game
    ///        threads as long as each target a distinct slot.
    void WriteTransform(ProxyId id, const TransformMat3x4& matrix, std::uint32_t flags = 0) const noexcept;

    /// @brief Publishes the current writing half and flips to the other half for the next
    ///        frame. Called by RenderSubsystem::SubmitFrame on the game thread.
    void PublishAndFlip(std::uint64_t frameId) noexcept;

    /// @brief Render-thread accessor: returns the bindless index of the half last published
    ///        before the supplied frameId, or kInvalidHalfIndex when nothing has been
    ///        published yet.
    [[nodiscard]] std::uint32_t AcquireReadHalfBindlessIndex(std::uint64_t frameId) const noexcept;

    [[nodiscard]] std::uint32_t GetCapacity() const noexcept { return capacity_; }
    [[nodiscard]] std::uint32_t GetWritingHalf() const noexcept { return writingHalf_; }
    [[nodiscard]] std::uint32_t GetLastPublishedHalf() const noexcept
    {
        return lastPublishedHalf_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::uint64_t GetLastPublishedFrameId() const noexcept
    {
        return lastPublishedFrameId_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsValid() const noexcept;

private:
    bool CreateHalves(RHI::IRHIDevice* device);

    std::uint32_t        capacity_{0};
    Ref<RHI::IRHIBuffer> halves_[2];
    TransformGpuRecord*  mapped_[2]{};
    std::uint32_t        bindlessIndices_[2]{kInvalidHalfIndex, kInvalidHalfIndex};

    std::atomic<std::uint32_t> nextSlot_{0};
    std::uint32_t              writingHalf_{0};
    std::atomic<std::uint32_t> lastPublishedHalf_{kInvalidHalfIndex};
    std::atomic<std::uint64_t> lastPublishedFrameId_{0};
};

} // namespace Hylux::Renderer
