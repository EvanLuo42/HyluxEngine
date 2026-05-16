/// @file
/// @brief Pipeline layout interface and descriptor. Bindless-first: every layout reserves
///        push constants and per-stage bindless heap pointers by default.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIPipelineDesc.h"

namespace Hylux::RHI
{

/// @brief Layout shared by pipelines bound under the same root signature / VK pipeline layout.
class IRHIPipelineLayout : public IRHIObject
{
public:
    /// @brief Returns the descriptor the layout was created with.
    [[nodiscard]] virtual const PipelineLayoutDesc& GetDesc() const noexcept = 0;
};

} // namespace Hylux::RHI
