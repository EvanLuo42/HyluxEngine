/// @file
/// @brief MaterialAsset implementation.

#include "Renderer/Material/MaterialAsset.h"

namespace Hylux::Renderer
{

const ParameterDesc* MaterialAsset::FindParameter(NameHash name) const noexcept
{
    for (const auto& desc : config_.parameters)
    {
        if (desc.name == name)
        {
            return &desc;
        }
    }
    return nullptr;
}

} // namespace Hylux::Renderer
