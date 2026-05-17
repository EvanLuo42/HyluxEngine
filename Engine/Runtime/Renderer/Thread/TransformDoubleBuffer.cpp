/// @file
/// @brief TransformDoubleBuffer implementation.

#include "Renderer/Thread/TransformDoubleBuffer.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIResourceDesc.h"

#include <cstring>

namespace Hylux::Renderer
{

TransformDoubleBuffer::TransformDoubleBuffer(RHI::IRHIDevice* device, std::uint32_t capacity) : capacity_(capacity)
{
    if (device == nullptr || capacity_ == 0)
    {
        HYLUX_LOG_ERROR(LogRender, "TransformDoubleBuffer: invalid construction (device={}, capacity={})",
                        device != nullptr, capacity_);
        return;
    }
    if (!CreateHalves(device))
    {
        HYLUX_LOG_ERROR(LogRender, "TransformDoubleBuffer: half allocation failed");
    }
}

TransformDoubleBuffer::~TransformDoubleBuffer()
{
    for (std::uint32_t i = 0; i < 2; ++i)
    {
        if (halves_[i] && mapped_[i] != nullptr)
        {
            halves_[i]->Unmap();
            mapped_[i] = nullptr;
        }
    }
}

bool TransformDoubleBuffer::CreateHalves(RHI::IRHIDevice* device)
{
    const std::uint64_t sizeBytes = static_cast<std::uint64_t>(capacity_) * sizeof(TransformGpuRecord);

    for (std::uint32_t i = 0; i < 2; ++i)
    {
        RHI::BufferDesc desc{};
        desc.size = sizeBytes;
        desc.usage = RHI::BufferUsage::StorageBuffer;
        desc.memory = RHI::MemoryUsage::CpuToGpu;
        desc.structureStride = sizeof(TransformGpuRecord);
        desc.bindlessSrv = true;

        halves_[i] = device->CreateBuffer(desc);
        if (!halves_[i])
        {
            return false;
        }

        void* cpu = halves_[i]->Map(0, RHI::kWholeSize);
        if (cpu == nullptr)
        {
            return false;
        }
        mapped_[i] = static_cast<TransformGpuRecord*>(cpu);
        std::memset(mapped_[i], 0, sizeBytes);

        const auto bindless = halves_[i]->GetBindlessIndex(RHI::BindlessKind::SrvCbvUav);
        bindlessIndices_[i] =
            (bindless == RHI::BindlessIndex::Invalid) ? kInvalidHalfIndex : static_cast<std::uint32_t>(bindless);
    }
    return true;
}

bool TransformDoubleBuffer::IsValid() const noexcept
{
    return halves_[0] && halves_[1] && mapped_[0] != nullptr && mapped_[1] != nullptr;
}

ProxyId TransformDoubleBuffer::AllocateSlot()
{
    const auto raw = nextSlot_.fetch_add(1, std::memory_order_acq_rel);
    if (raw >= capacity_)
    {
        HYLUX_LOG_ERROR(LogRender, "TransformDoubleBuffer: capacity exhausted (capacity={}, requested={})", capacity_,
                        raw);
        return ProxyId::Invalid;
    }
    return static_cast<ProxyId>(raw);
}

void TransformDoubleBuffer::ReleaseSlot(ProxyId /*id*/) noexcept
{
    // Stage 4a: slot leak by design. A free list with deferred retirement lands when stage
    // 5+ stresses capacity. The proxy entry is still removed from ProxyRegistry, so future
    // frames simply skip the slot.
}

void TransformDoubleBuffer::WriteTransform(ProxyId id, const TransformMat3x4& matrix, std::uint32_t flags) const noexcept
{
    if (!IsValid())
    {
        return;
    }
    const auto slot = static_cast<std::uint32_t>(id);
    if (slot >= capacity_)
    {
        return;
    }
    TransformGpuRecord& record = mapped_[writingHalf_][slot];
    matrix.StoreRowMajor(record.worldMatrix);
    record.flags = flags;
}

void TransformDoubleBuffer::PublishAndFlip(std::uint64_t frameId) noexcept
{
    const auto published = writingHalf_;
    lastPublishedFrameId_.store(frameId, std::memory_order_release);
    lastPublishedHalf_.store(published, std::memory_order_release);
    writingHalf_ ^= 1u;
}

std::uint32_t TransformDoubleBuffer::AcquireReadHalfBindlessIndex(std::uint64_t /*frameId*/) const noexcept
{
    const auto half = lastPublishedHalf_.load(std::memory_order_acquire);
    if (half == kInvalidHalfIndex)
    {
        return kInvalidHalfIndex;
    }
    return bindlessIndices_[half];
}

} // namespace Hylux::Renderer
