/// @file
/// @brief Base class for all RenderGraph passes. A pass declares its resource dependencies in
///        Setup via the supplied builder, then records GPU work in Execute.

#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Hylux::RG
{

class RGBuilder;
class RGContext;
class RenderGraph;

/// @brief High-level placement preference for a pass. Drives which physical queue the future
///        scheduler picks. `Graphics` is the only mode the current single-queue executor
///        respects; `AsyncCompute` and `Copy` are recorded on the pass node so a future
///        multi-queue submission path can split the graph without touching call sites.
enum class QueueAffinity : std::uint8_t
{
    Graphics = 0,
    AsyncCompute,
    Copy,
};

/// @brief Coarse identification of a pass's workload kind. Stored on the pass node by
///        RenderGraph::AddPass. RGPass subclasses do not need to override anything; the graph
///        derives the kind from the C++ type at registration time.
enum class RGPassKind : std::uint8_t
{
    Generic = 0,
    Raster,
    Compute,
    RayTracing,
    Copy,
};

/// @brief Abstract base for RenderGraph passes. Subclasses override Setup to declare reads,
///        writes, and (for raster passes) attachments through the builder; Execute records the
///        actual command list work. Subclasses store handles as members between the two calls.
class RGPass
{
    HYLUX_REFLECT_BODY(RGPass)
public:
    RGPass() = default;
    virtual ~RGPass() = default;

    RGPass(const RGPass&)            = delete;
    RGPass& operator=(const RGPass&) = delete;
    RGPass(RGPass&&)                 = delete;
    RGPass& operator=(RGPass&&)      = delete;

    /// @brief Declares resource dependencies through the builder. Called exactly once,
    ///        immediately after the pass is added to a RenderGraph.
    virtual void Setup(RGBuilder& builder) = 0;

    /// @brief Records GPU work onto the supplied command list. Called during RenderGraph::Execute
    ///        after the pass's prelude barriers (and BeginRendering for raster passes) have run.
    virtual void Execute(RGContext& context, RHI::IRHICommandList& cmd) = 0;

    /// @brief Preferred physical queue for this pass. Defaults to graphics; compute / copy
    ///        subclasses override. The executor is allowed to fall back to graphics when the
    ///        device lacks a dedicated queue for the requested affinity.
    [[nodiscard]] virtual QueueAffinity GetQueueAffinity() const noexcept
    {
        return QueueAffinity::Graphics;
    }

    /// @brief Returns the debug name assigned at AddPass time.
    [[nodiscard]] std::string_view GetName() const noexcept { return name_; }

private:
    friend class RenderGraph;
    std::string name_;
};

} // namespace Hylux::RG
