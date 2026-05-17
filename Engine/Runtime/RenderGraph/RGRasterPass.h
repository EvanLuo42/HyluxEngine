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
};

} // namespace Hylux::RG
