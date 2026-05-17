/// @file
/// @brief Per-frame bump arena. Owns a growable chunk list; Allocate() advances a single
///        offset, Reset() rewinds every chunk to zero without releasing memory. Built for
///        the engine's single-threaded main loop — no internal locking, callers must
///        serialise access. The chunk list is grown but never shrunk during a frame, so
///        the working set stabilises at the high-water mark of all frames seen so far.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux
{

/// @brief Growable single-threaded bump allocator. Allocations live until Reset(); no
///        per-allocation deallocate. The default chunk size sets the first chunk and the
///        minimum size of every subsequent chunk grown on overflow.
class FrameAllocator
{
public:
    static constexpr std::size_t kDefaultChunkSize = 64 * 1024;

    explicit FrameAllocator(std::size_t initialChunkBytes = kDefaultChunkSize);

    FrameAllocator(const FrameAllocator&)            = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;

    // Non-movable because the embedded pmr adapter holds a back-pointer to *this. The
    // arena is meant to live as a long-lived member of the owning subsystem.
    FrameAllocator(FrameAllocator&&)            = delete;
    FrameAllocator& operator=(FrameAllocator&&) = delete;

    ~FrameAllocator() = default;

    /// @brief Reserves `size` bytes aligned to `alignment` (a power of two). Walks the
    ///        chunk list and, if every chunk is too full, allocates a fresh chunk sized
    ///        to max(defaultChunkSize_, size + alignment). Returns a pointer that stays
    ///        valid until the next Reset().
    [[nodiscard]] void* Allocate(std::size_t size, std::size_t alignment);

    /// @brief Constructs a single T using the arena. The caller is responsible for
    ///        calling T's destructor before Reset() — TrivialDestructor types and
    ///        callers that handle teardown explicitly are the intended consumers.
    template<typename T, typename... Args>
    [[nodiscard]] T* Construct(Args&&... args)
    {
        void* memory = Allocate(sizeof(T), alignof(T));
        return ::new (memory) T(std::forward<Args>(args)...);
    }

    /// @brief Allocates an uninitialised array of N T's. Caller initialises in place.
    template<typename T>
    [[nodiscard]] T* AllocateArray(std::size_t count)
    {
        return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    /// @brief Rewinds every chunk to zero used bytes, keeping the chunk list intact so
    ///        the next frame reuses the same memory. The high-water mark is preserved.
    void Reset() noexcept;

    [[nodiscard]] std::size_t BytesUsedThisFrame() const noexcept { return bytesUsedThisFrame_; }
    [[nodiscard]] std::size_t BytesCapacity() const noexcept;
    [[nodiscard]] std::size_t HighWaterMark() const noexcept { return highWaterMark_; }
    [[nodiscard]] std::size_t ChunkCount() const noexcept { return chunks_.size(); }

    /// @brief Returns a std::pmr::memory_resource view of this arena suitable for
    ///        std::pmr::vector and other polymorphic-allocator containers. Two views of
    ///        the same arena compare equal; views of different arenas compare unequal.
    [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() noexcept { return &pmrResource_; }

    /// @brief STL-compatible allocator that routes all allocations through a parent
    ///        FrameAllocator. Deallocate is a no-op (the arena reclaims everything at
    ///        Reset()). Non-propagating, non-equal across distinct parents.
    template<typename T>
    class StlAdapter
    {
    public:
        using value_type = T;

        explicit StlAdapter(FrameAllocator* parent) noexcept : parent_(parent) {}

        template<typename U>
        StlAdapter(const StlAdapter<U>& other) noexcept : parent_(other.parent_)
        {}

        [[nodiscard]] T* allocate(std::size_t count)
        {
            return static_cast<T*>(parent_->Allocate(count * sizeof(T), alignof(T)));
        }

        void deallocate(T*, std::size_t) noexcept {}

        [[nodiscard]] bool operator==(const StlAdapter& other) const noexcept
        {
            return parent_ == other.parent_;
        }
        [[nodiscard]] bool operator!=(const StlAdapter& other) const noexcept
        {
            return parent_ != other.parent_;
        }

        template<typename U> friend class StlAdapter;

    private:
        FrameAllocator* parent_;
    };

private:
    struct Chunk
    {
        std::unique_ptr<std::byte[]> data;
        std::size_t                  capacity;
        std::size_t                  used;
    };

    /// @brief std::pmr adapter forwarding allocate to the owning arena. Deallocate is a
    ///        no-op; identity is by parent pointer.
    class PmrResource final : public std::pmr::memory_resource
    {
    public:
        explicit PmrResource(FrameAllocator* parent) noexcept : parent_(parent) {}

    private:
        [[nodiscard]] void* do_allocate(std::size_t bytes, std::size_t alignment) override
        {
            return parent_->Allocate(bytes, alignment);
        }
        void do_deallocate(void*, std::size_t, std::size_t) override {}
        [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            const auto* o = dynamic_cast<const PmrResource*>(&other);
            return o != nullptr && o->parent_ == parent_;
        }

        FrameAllocator* parent_;
    };

    void GrowAtLeast(std::size_t requiredBytes);

    std::vector<Chunk> chunks_;
    std::size_t        defaultChunkSize_;
    std::size_t        currentChunkIndex_{0};
    std::size_t        bytesUsedThisFrame_{0};
    std::size_t        highWaterMark_{0};
    PmrResource        pmrResource_{this};
};

} // namespace Hylux
