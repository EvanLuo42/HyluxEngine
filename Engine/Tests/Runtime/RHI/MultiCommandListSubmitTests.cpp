/// @file
/// @brief Confirms that IRHIQueue::Submit accepts an ordered span of primary command lists
///        produced from independent IRHICommandPool instances. This is the existing API the
///        TaskGraph-driven parallel renderpass recording (Slice H) depends on; the test
///        guards against regressions to the queue submit surface.

#include "../RHI/GpuCase.h"

#include "Core/Memory/Ref.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHICommandPool.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIQueue.h"

#include <doctest/doctest.h>

#include <array>

TEST_SUITE_BEGIN("RHI");

using namespace Hylux;
using namespace Hylux::Tests;

GPU_CASE("Queue::Submit accepts span of primary CLs from independent pools", Backends::All)
{
    Ref<RHI::IRHIQueue> queue = gpu.device->GetQueue(RHI::QueueType::Graphics, 0);
    REQUIRE(queue);

    Ref<RHI::IRHICommandPool> poolA =
        gpu.device->CreateCommandPool(RHI::QueueType::Graphics, RHI::CommandPoolFlagBits::AllowIndividualReset);
    Ref<RHI::IRHICommandPool> poolB =
        gpu.device->CreateCommandPool(RHI::QueueType::Graphics, RHI::CommandPoolFlagBits::AllowIndividualReset);
    REQUIRE(poolA);
    REQUIRE(poolB);

    Ref<RHI::IRHICommandList> clA = poolA->AllocateCommandList();
    Ref<RHI::IRHICommandList> clB = poolB->AllocateCommandList();
    REQUIRE(clA);
    REQUIRE(clB);

    clA->Begin();
    clA->End();
    clB->Begin();
    clB->End();

    std::array<RHI::IRHICommandList*, 2> lists{clA.Get(), clB.Get()};
    RHI::SubmitDesc                      submit{};
    submit.commandLists = lists;

    std::array<RHI::SubmitDesc, 1> batches{submit};
    CHECK(queue->Submit(batches));
    CHECK(queue->WaitIdle());
}

TEST_SUITE_END();
