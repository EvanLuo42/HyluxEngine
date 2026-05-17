/// @file
/// @brief Tests for Hylux::FrameAllocator.

#include "Core/Memory/FrameAllocator.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <memory_resource>
#include <vector>

TEST_SUITE_BEGIN("Core::Memory");

using namespace Hylux;

namespace
{

[[nodiscard]] bool IsAligned(const void* pointer, std::size_t alignment) noexcept
{
    return (reinterpret_cast<std::uintptr_t>(pointer) & (alignment - 1)) == 0;
}

} // namespace

TEST_CASE("FrameAllocator: basic allocation returns aligned, distinct pointers")
{
    FrameAllocator arena(1024);

    void* a = arena.Allocate(16, 16);
    void* b = arena.Allocate(32, 16);
    void* c = arena.Allocate(7, 8);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    CHECK(IsAligned(a, 16));
    CHECK(IsAligned(b, 16));
    CHECK(IsAligned(c, 8));
    CHECK(a != b);
    CHECK(b != c);
    CHECK(arena.BytesUsedThisFrame() >= 16u + 32u + 7u);
}

TEST_CASE("FrameAllocator: Reset rewinds the offset, memory is reused")
{
    FrameAllocator arena(512);

    void* a = arena.Allocate(64, 8);
    CHECK(arena.BytesUsedThisFrame() >= 64u);

    arena.Reset();
    CHECK(arena.BytesUsedThisFrame() == 0u);

    void* b = arena.Allocate(64, 8);
    CHECK(b == a);
}

TEST_CASE("FrameAllocator: grows a fresh chunk when the current chunk overflows")
{
    FrameAllocator arena(128);
    CHECK(arena.ChunkCount() == 1u);

    void* first = arena.Allocate(64, 8);
    REQUIRE(first != nullptr);
    void* huge = arena.Allocate(4096, 16);
    REQUIRE(huge != nullptr);
    CHECK(arena.ChunkCount() == 2u);
    CHECK(IsAligned(huge, 16));
    CHECK(arena.BytesCapacity() >= 128u + 4096u);
}

TEST_CASE("FrameAllocator: HighWaterMark persists across Reset")
{
    FrameAllocator arena(1024);
    arena.Allocate(800, 8);
    const std::size_t peak = arena.BytesUsedThisFrame();
    CHECK(peak >= 800u);

    arena.Reset();
    CHECK(arena.BytesUsedThisFrame() == 0u);
    CHECK(arena.HighWaterMark() == peak);

    arena.Allocate(64, 8);
    CHECK(arena.HighWaterMark() == peak);
}

TEST_CASE("FrameAllocator: Construct calls T's constructor in arena memory")
{
    struct Point { int x; int y; };

    FrameAllocator arena(256);
    Point*         p = arena.Construct<Point>(3, 4);
    REQUIRE(p != nullptr);
    CHECK(p->x == 3);
    CHECK(p->y == 4);
    CHECK(IsAligned(p, alignof(Point)));
}

TEST_CASE("FrameAllocator: StlAdapter routes std::vector through the arena")
{
    FrameAllocator arena(4096);
    {
        std::vector<int, FrameAllocator::StlAdapter<int>> values{FrameAllocator::StlAdapter<int>(&arena)};
        for (int i = 0; i < 64; ++i)
        {
            values.push_back(i);
        }
        CHECK(values.size() == 64u);
        for (int i = 0; i < 64; ++i)
        {
            CHECK(values[static_cast<std::size_t>(i)] == i);
        }
        CHECK(arena.BytesUsedThisFrame() >= 64u * sizeof(int));
    }
    arena.Reset();
    CHECK(arena.BytesUsedThisFrame() == 0u);
}

TEST_CASE("FrameAllocator: pmr adapter backs std::pmr::vector")
{
    FrameAllocator arena(4096);
    {
        std::pmr::vector<int> values{arena.GetMemoryResource()};
        for (int i = 0; i < 100; ++i)
        {
            values.push_back(i * 2);
        }
        CHECK(values.size() == 100u);
        CHECK(values[50] == 100);
        CHECK(arena.BytesUsedThisFrame() >= 100u * sizeof(int));
    }
    arena.Reset();
    CHECK(arena.BytesUsedThisFrame() == 0u);
}

TEST_CASE("FrameAllocator: pmr adapter is_equal compares parents")
{
    FrameAllocator a(256);
    FrameAllocator b(256);
    CHECK(a.GetMemoryResource()->is_equal(*a.GetMemoryResource()));
    CHECK_FALSE(a.GetMemoryResource()->is_equal(*b.GetMemoryResource()));
}

TEST_SUITE_END();
