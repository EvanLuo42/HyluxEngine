/// @file
/// @brief Execute-time handle handed to RendererRasterPass::ExecuteRenderer. Sole authoring
///        surface for issuing draws against a built DrawList. The escape hatch GetCommandList
///        is provided for one-off recording the renderer does not yet abstract; use sparingly.

#pragma once

#include "RHI/RHIForward.h"
#include "Renderer/Pso/PsoRequest.h"
#include "Renderer/RendererForward.h"

#include <cstdint>

namespace Hylux::RG
{
class RGContext;
}

namespace Hylux::Renderer
{

class SceneView;

/// @brief Per-pass execution context. Owned by RendererRasterPass during the call to the
///        user's ExecuteRenderer override; do not retain past return.
class RendererPassContext
{
public:
    RendererPassContext(RG::RGContext& rgContext, RHI::IRHICommandList& cmd, const SceneView& view, PsoCache* psoCache,
                        std::uint32_t transformBindlessIndex, const PsoFormatKey& formats) noexcept;

    RendererPassContext(const RendererPassContext&) = delete;
    RendererPassContext& operator=(const RendererPassContext&) = delete;
    RendererPassContext(RendererPassContext&&) = delete;
    RendererPassContext& operator=(RendererPassContext&&) = delete;

    /// @brief Issues a previously-built DrawList: resolves the PSO via PsoCache, binds the
    ///        layout + pipeline, writes the canonical push-constant block, and emits a
    ///        DrawIndirectCount when the list has non-zero draws. Safe no-op on empty list
    ///        / PSO resolution failure (e.g. missing shader archive entry).
    void DrawRendererList(const DrawList& list);

    /// @brief Escape hatch. Authors may record arbitrary RHI commands when no higher-level
    ///        primitive exists. Avoid unless strictly necessary.
    [[nodiscard]] RHI::IRHICommandList& GetCommandList() noexcept { return cmd_; }

    [[nodiscard]] const SceneView&    GetView() const noexcept { return view_; }
    [[nodiscard]] RG::RGContext&      GetRGContext() const noexcept { return rgContext_; }
    [[nodiscard]] PsoCache*           GetPsoCache() const noexcept { return psoCache_; }
    [[nodiscard]] std::uint32_t       GetTransformBindlessIndex() const noexcept { return transformBindlessIndex_; }
    [[nodiscard]] const PsoFormatKey& GetFormats() const noexcept { return formats_; }

private:
    RG::RGContext&        rgContext_;
    RHI::IRHICommandList& cmd_;
    const SceneView&      view_;
    PsoCache*             psoCache_{nullptr};
    std::uint32_t         transformBindlessIndex_{0xFFFFFFFFu};
    PsoFormatKey          formats_{};
};

} // namespace Hylux::Renderer
