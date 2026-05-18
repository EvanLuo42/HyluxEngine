/// @file
/// @brief Reflection registration for RGComputePass / RGAsyncComputePass.

#include "RenderGraph/RGComputePass.h"

HYLUX_REFLECT_CLASS_BEGIN(Hylux::RG::RGComputePass)
HYLUX_REFLECT_BASE(Hylux::RG::RGPass)
HYLUX_REFLECT_CLASS_END(Hylux::RG::RGComputePass)

HYLUX_REFLECT_CLASS_BEGIN(Hylux::RG::RGAsyncComputePass)
HYLUX_REFLECT_BASE(Hylux::RG::RGComputePass)
HYLUX_REFLECT_CLASS_END(Hylux::RG::RGAsyncComputePass)
