/// @file
/// @brief RGRasterPass bridge implementation and reflection registration.

#include "RenderGraph/RGRasterPass.h"

#include "RenderGraph/RGRasterBuilder.h"

#include <cassert>

namespace Hylux::RG
{

void RGRasterPass::Setup(RGBuilder& builder)
{
    auto* raster = static_cast<RGRasterBuilder*>(&builder);
    assert(raster != nullptr && "RGRasterPass::Setup received a non-raster builder");
    Setup(*raster);
}

} // namespace Hylux::RG

HYLUX_REFLECT_CLASS_BEGIN(Hylux::RG::RGRasterPass)
HYLUX_REFLECT_BASE(Hylux::RG::RGPass)
HYLUX_REFLECT_CLASS_END(Hylux::RG::RGRasterPass)
