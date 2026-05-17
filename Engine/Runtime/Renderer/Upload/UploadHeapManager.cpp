/// @file
/// @brief UploadHeapManager implementation.

#include "Renderer/Upload/UploadHeapManager.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIResourceDesc.h"

namespace Hylux::Renderer
{
namespace
{

[[nodiscard]] std::uint64_t AlignUp(const std::uint64_t value, const std::uint64_t alignment) noexcept
{
    return alignment <= 1 ? value : ((value + alignment - 1) & ~(alignment - 1));
}

} // namespace

UploadHeapManager::UploadHeapManager(RHI::IRHIDevice* device, std::uint32_t framesInFlight, std::uint64_t bytesPerFrame)
    : framesInFlight_(framesInFlight == 0 ? 1u : framesInFlight), bytesPerFrame_(bytesPerFrame)
{
    if (device == nullptr || bytesPerFrame_ == 0)
    {
        HYLUX_LOG_ERROR(LogRender, "UploadHeapManager: invalid construction (device={}, bytesPerFrame={})",
                        device != nullptr, bytesPerFrame_);
        return;
    }

    RHI::BufferDesc desc{};
    desc.size = static_cast<std::uint64_t>(framesInFlight_) * bytesPerFrame_;
    desc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::IndirectBuffer | RHI::BufferUsage::TransferSrc;
    desc.memory = RHI::MemoryUsage::CpuToGpu;
    desc.structureStride = 0;
    desc.bindlessSrv = true;

    buffer_ = device->CreateBuffer(desc);
    if (!buffer_)
    {
        HYLUX_LOG_ERROR(LogRender, "UploadHeapManager: buffer allocation failed (size={} bytes)", desc.size);
        return;
    }

    mapped_ = buffer_->Map(0, RHI::kWholeSize);
    if (mapped_ == nullptr)
    {
        HYLUX_LOG_ERROR(LogRender, "UploadHeapManager: persistent map failed");
        buffer_.Reset();
        return;
    }

    const auto idx = buffer_->GetBindlessIndex(RHI::BindlessKind::SrvCbvUav);
    bindlessIndex_ = idx == RHI::BindlessIndex::Invalid ? kInvalidBindlessIndex : static_cast<std::uint32_t>(idx);

    // Cursor starts at slot 0's base; the first BeginFrame call may override.
    currentFrameSlot_ = 0;
    currentOffset_ = 0;
}

UploadHeapManager::~UploadHeapManager()
{
    if (buffer_ && mapped_ != nullptr)
    {
        buffer_->Unmap();
        mapped_ = nullptr;
    }
}

bool UploadHeapManager::IsValid() const noexcept
{
    return static_cast<bool>(buffer_) && mapped_ != nullptr;
}

UploadHeapManager::Allocation UploadHeapManager::Allocate(std::uint64_t size, std::uint64_t alignment) noexcept
{
    if (!IsValid() || size == 0)
    {
        return {};
    }

    const std::uint64_t slotBase = static_cast<std::uint64_t>(currentFrameSlot_) * bytesPerFrame_;
    const std::uint64_t slotEnd = slotBase + bytesPerFrame_;

    const std::uint64_t aligned = AlignUp(currentOffset_, alignment == 0 ? 1ull : alignment);
    if (aligned + size > slotEnd)
    {
        HYLUX_LOG_WARN(LogRender,
                       "UploadHeapManager: per-frame ring exhausted (slot={}, requested={}, "
                       "remaining={})",
                       currentFrameSlot_, size, slotEnd - aligned);
        return {};
    }

    Allocation a{};
    a.cpu = static_cast<char*>(mapped_) + aligned;
    a.gpu = buffer_.Get();
    a.offset = aligned;
    a.bindlessIndex = bindlessIndex_;
    currentOffset_ = aligned + size;
    return a;
}

void UploadHeapManager::BeginFrame(const std::uint32_t frameSlot) noexcept
{
    currentFrameSlot_ = frameSlot % framesInFlight_;
    currentOffset_ = static_cast<std::uint64_t>(currentFrameSlot_) * bytesPerFrame_;
}

} // namespace Hylux::Renderer
