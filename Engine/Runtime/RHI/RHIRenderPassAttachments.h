/// @file
/// @brief Format-only descriptor of a renderpass instance. Used by callers that need to
///        describe the *shape* of a renderpass (formats, sample count, view mask) without
///        binding actual attachment views. Examples: pipeline-rendering creation, secondary
///        command list inheritance, parallel-recording renderpass-batch identity.

#pragma once

#include "RHI/RHIFormat.h"
#include "RHI/RHITypes.h"

#include <array>
#include <cstdint>

namespace Hylux::RHI
{

/// @brief Format-level description of a renderpass instance. Two passes share the same
///        renderpass shape (and can therefore record into the same renderpass batch) if and
///        only if their RHIRenderPassAttachments are equal.
struct RHIRenderPassAttachments
{
    std::array<Format, kMaxColorAttachments> colorFormats{};
    std::uint32_t                            colorAttachmentCount{0};
    Format                                   depthFormat{Format::Unknown};
    Format                                   stencilFormat{Format::Unknown};
    std::uint32_t                            sampleCount{1};
    std::uint32_t                            viewMask{0};

    [[nodiscard]] friend bool operator==(const RHIRenderPassAttachments& a,
                                         const RHIRenderPassAttachments& b) noexcept
    {
        if (a.colorAttachmentCount != b.colorAttachmentCount) { return false; }
        if (a.depthFormat   != b.depthFormat)   { return false; }
        if (a.stencilFormat != b.stencilFormat) { return false; }
        if (a.sampleCount   != b.sampleCount)   { return false; }
        if (a.viewMask      != b.viewMask)      { return false; }
        for (std::uint32_t i = 0; i < a.colorAttachmentCount; ++i)
        {
            if (a.colorFormats[i] != b.colorFormats[i]) { return false; }
        }
        return true;
    }

    [[nodiscard]] friend bool operator!=(const RHIRenderPassAttachments& a,
                                         const RHIRenderPassAttachments& b) noexcept
    {
        return !(a == b);
    }
};

} // namespace Hylux::RHI
