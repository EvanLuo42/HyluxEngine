/// @file
/// @brief MaterialAsset implementation. Pure metadata accessors today; lookup-by-name
///        is a linear scan because the parameter list is short by construction (one
///        entry per shader uniform / texture binding).

#include "Asset/Types/MaterialAsset.h"

namespace Hylux::Asset
{

const ParameterDesc* MaterialAsset::FindParameter(NameHash name) const noexcept
{
    for (const auto& desc : data_.parameters)
    {
        if (desc.name == name)
        {
            return &desc;
        }
    }
    return nullptr;
}

} // namespace Hylux::Asset
