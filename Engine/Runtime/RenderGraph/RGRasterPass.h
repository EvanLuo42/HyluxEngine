/// @file
/// @brief Raster-style RenderGraph pass: declares color/depth attachments alongside reads/writes
///        and is automatically wrapped in BeginRendering/EndRendering during Execute.

#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "RenderGraph/RGPass.h"

namespace Hylux::RG
{

class RGRasterBuilder;

/// @brief Specialization of RGPass for rasterization work. Subclasses implement
///        Setup(RGRasterBuilder&) instead of Setup(RGBuilder&); RenderGraph forwards the
///        correctly-typed builder. Execute is invoked between cmd.BeginRendering /
///        cmd.EndRendering with the recorded attachments.
class RGRasterPass : public RGPass
{
    HYLUX_REFLECT_BODY(RGRasterPass)
public:
    /// @brief Sealed bridge. Forwards to the typed Setup overload; not overridable by users.
    void Setup(RGBuilder& builder) final;

    /// @brief Overridden by user pass subclasses to declare attachments and dependencies.
    virtual void Setup(RGRasterBuilder& builder) = 0;

    /// @brief Advisory hook for tile-memory / subpass merging. When this pass is added
    ///        immediately after `previous`, the compiler asks `this->CanMergeWith(previous)`
    ///        whether the two should share a render-pass instance — keeping attachments in
    ///        tile memory between them on mobile / console GPUs and avoiding an unnecessary
    ///        store/load round-trip on desktop. The default answer is `false`. Subclasses
    ///        that want to be mergeable should return `true` only when they read the
    ///        previous pass's color/depth output as an input attachment via
    ///        RGRasterBuilder::ReadInputAttachment and write the same render area.
    ///        The current executor records the answer on the pass node but does not act on
    ///        it; reserved so the merger can land without churn on pass call sites.
    [[nodiscard]] virtual bool CanMergeWith(const RGRasterPass& previous) const noexcept
    {
        (void)previous;
        return false;
    }
};

} // namespace Hylux::RG
