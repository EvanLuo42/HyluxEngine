/// @file
/// @brief Texture sampler interface.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIResourceDesc.h"

namespace Hylux::RHI
{

/// @brief Sampler state object. Samplers are always allocated into the bindless sampler
///        heap on creation since the heap capacity is small enough to track all samplers.
class IRHISampler : public IRHIObject
{
public:
    /// @brief Returns the descriptor the sampler was created with.
    [[nodiscard]] virtual const SamplerDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the bindless table index of this sampler within the Sampler heap.
    [[nodiscard]] virtual BindlessIndex GetBindlessIndex() const noexcept = 0;
};

} // namespace Hylux::RHI
