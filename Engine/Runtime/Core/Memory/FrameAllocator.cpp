/// @file
/// @brief FrameAllocator implementation. See the header for invariants.

#include "Core/Memory/FrameAllocator.h"

#include <cassert>
#include <new>

namespace Hylux
{

namespace
{

[[nodiscard]] std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept
{
    return (value + alignment - 1) & ~(alignment - 1);
}

[[nodiscard]] bool IsPowerOfTwo(std::size_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

} // namespace

FrameAllocator::FrameAllocator(std::size_t initialChunkBytes)
    : defaultChunkSize_(initialChunkBytes == 0 ? kDefaultChunkSize : initialChunkBytes)
{
    GrowAtLeast(defaultChunkSize_);
}

void* FrameAllocator::Allocate(std::size_t size, std::size_t alignment)
{
    assert(IsPowerOfTwo(alignment) && "FrameAllocator: alignment must be a power of two");
    if (size == 0)
    {
        size = 1;
    }

    for (std::size_t i = currentChunkIndex_; i < chunks_.size(); ++i)
    {
        Chunk&            chunk          = chunks_[i];
        std::byte*        base           = chunk.data.get();
        const std::size_t alignedOffset  = AlignUp(reinterpret_cast<std::uintptr_t>(base + chunk.used),
                                                   alignment)
                                          - reinterpret_cast<std::uintptr_t>(base);
        if (alignedOffset + size <= chunk.capacity)
        {
            const std::size_t consumed = (alignedOffset - chunk.used) + size;
            chunk.used                 = alignedOffset + size;
            currentChunkIndex_         = i;
            bytesUsedThisFrame_ += consumed;
            if (bytesUsedThisFrame_ > highWaterMark_)
            {
                highWaterMark_ = bytesUsedThisFrame_;
            }
            return base + alignedOffset;
        }
    }

    GrowAtLeast(size + alignment);
    return Allocate(size, alignment);
}

void FrameAllocator::Reset() noexcept
{
    for (Chunk& chunk : chunks_)
    {
        chunk.used = 0;
    }
    currentChunkIndex_  = 0;
    bytesUsedThisFrame_ = 0;
}

std::size_t FrameAllocator::BytesCapacity() const noexcept
{
    std::size_t total = 0;
    for (const Chunk& chunk : chunks_)
    {
        total += chunk.capacity;
    }
    return total;
}

void FrameAllocator::GrowAtLeast(std::size_t requiredBytes)
{
    const std::size_t chunkBytes = requiredBytes > defaultChunkSize_ ? requiredBytes : defaultChunkSize_;
    Chunk             chunk{};
    chunk.data     = std::unique_ptr<std::byte[]>(new std::byte[chunkBytes]);
    chunk.capacity = chunkBytes;
    chunk.used     = 0;
    chunks_.push_back(std::move(chunk));
    currentChunkIndex_ = chunks_.size() - 1;
}

} // namespace Hylux
