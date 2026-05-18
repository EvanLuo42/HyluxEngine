/// @file
/// @brief Compute-style RenderGraph passes. RGComputePass runs on the graphics queue by
///        default; RGAsyncComputePass requests the dedicated compute queue so the scheduler
///        can overlap it with graphics work.

#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "RenderGraph/RGPass.h"

namespace Hylux::RG
{

/// @brief Specialization of RGPass for compute work. The base Setup(RGBuilder&) surface is
///        sufficient — compute passes only need ReadBuffer/WriteBuffer/ReadTexture/WriteTexture
///        plus push constants and dispatch in Execute. Subclasses may override
///        GetQueueAffinity to request async compute on a per-instance basis without changing
///        the type.
class RGComputePass : public RGPass
{
    HYLUX_REFLECT_BODY(RGComputePass)
public:
    [[nodiscard]] QueueAffinity GetQueueAffinity() const noexcept override
    {
        return QueueAffinity::Graphics;
    }
};

/// @brief Compute pass pinned to the AsyncCompute queue. Use this when the pass is provably
///        independent of in-flight graphics work for its lifetime and overlap is desirable
///        (e.g. light culling alongside shadow rasterization, GI build alongside g-buffer).
///        Cross-queue synchronization is recorded by the graph at Compile time; on devices
///        without a dedicated compute queue the scheduler degrades to the graphics queue.
class RGAsyncComputePass : public RGComputePass
{
    HYLUX_REFLECT_BODY(RGAsyncComputePass)
public:
    [[nodiscard]] QueueAffinity GetQueueAffinity() const noexcept override
    {
        return QueueAffinity::AsyncCompute;
    }
};

} // namespace Hylux::RG
