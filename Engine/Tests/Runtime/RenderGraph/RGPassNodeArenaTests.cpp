/// @file
/// @brief Tests that RGPassNode routes its per-pass vector allocations through the
///        std::pmr::memory_resource it was constructed with, and that the default
///        constructor stays on the heap resource. Verifies the FrameAllocator → RG
///        wiring at the node level without standing up an IRHIDevice.

#include "Core/Memory/FrameAllocator.h"
#include "RenderGraph/Internal/RGNode.h"

#include <doctest/doctest.h>

#include <memory_resource>
#include <utility>

TEST_SUITE_BEGIN("RenderGraph");

using namespace Hylux;
using namespace Hylux::RG::Internal;

TEST_CASE("RGPassNode: explicit-resource ctor routes inner vector allocations to that resource")
{
    FrameAllocator arena(4096);
    REQUIRE(arena.BytesUsedThisFrame() == 0u);

    {
        RGPassNode node{arena.GetMemoryResource()};
        for (int i = 0; i < 32; ++i)
        {
            node.preludeTextureBarriers.push_back(RHI::TextureBarrier{});
            node.preludeBufferBarriers.push_back(RHI::BufferBarrier{});
        }
    }

    CHECK(arena.BytesUsedThisFrame() > 0u);
}

TEST_CASE("RGPassNode: default ctor uses the default heap resource (arena untouched)")
{
    FrameAllocator arena(4096);

    {
        RGPassNode node;
        for (int i = 0; i < 32; ++i)
        {
            node.preludeTextureBarriers.push_back(RHI::TextureBarrier{});
        }
    }

    CHECK(arena.BytesUsedThisFrame() == 0u);
}

TEST_CASE("RGPassNode: move-construction preserves the source's pmr resource")
{
    FrameAllocator arena(4096);

    RGPassNode source{arena.GetMemoryResource()};
    source.preludeTextureBarriers.push_back(RHI::TextureBarrier{});
    const std::size_t bytesAfterSourcePush = arena.BytesUsedThisFrame();
    CHECK(bytesAfterSourcePush > 0u);

    RGPassNode moved = std::move(source);
    moved.preludeTextureBarriers.push_back(RHI::TextureBarrier{});
    moved.preludeBufferBarriers.push_back(RHI::BufferBarrier{});

    // After the move, additional pushes on `moved` should still hit the arena (allocator
    // followed the move). If they had silently switched to heap, BytesUsedThisFrame would
    // stay at bytesAfterSourcePush.
    CHECK(arena.BytesUsedThisFrame() > bytesAfterSourcePush);
}

TEST_CASE("RGPassNode: textureAccesses / bufferAccesses / colorAttachments also follow the resource")
{
    FrameAllocator arena(4096);

    {
        RGPassNode node{arena.GetMemoryResource()};
        for (int i = 0; i < 16; ++i)
        {
            node.textureAccesses.push_back(RGTextureAccessRecord{});
            node.bufferAccesses.push_back(RGBufferAccessRecord{});
            node.colorAttachments.push_back(RGColorAttachmentRecord{});
        }
    }

    CHECK(arena.BytesUsedThisFrame() > 0u);
}

TEST_SUITE_END();
