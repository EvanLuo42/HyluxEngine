/// @file
/// @brief GPU texture and texture view interfaces.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

namespace Hylux::RHI
{

/// @brief Owning handle to a GPU texture resource. Default views are accessible via
///        GetBindlessIndex; named views require CreateTextureView.
class IRHITexture : public IRHIObject
{
public:
    /// @brief Returns the descriptor the texture was created with.
    [[nodiscard]] virtual const TextureDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the bindless table index for the texture's default whole-resource view,
    ///        or BindlessIndex::Invalid if the texture's usage does not include the
    ///        corresponding bindless capability.
    [[nodiscard]] virtual BindlessIndex GetBindlessIndex(BindlessKind kind) const noexcept = 0;
};

/// @brief View into a subset of an IRHITexture's mip and array range with an optional format
///        reinterpretation. Owning reference to the parent texture is held internally.
class IRHITextureView : public IRHIObject
{
public:
    /// @brief Returns the descriptor the view was created with.
    [[nodiscard]] virtual const TextureViewDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the parent texture. Always non-null.
    [[nodiscard]] virtual IRHITexture* GetTexture() const noexcept = 0;

    /// @brief Returns the bindless table index for this view, or BindlessIndex::Invalid
    ///        if the parent texture was not registered for bindless access.
    [[nodiscard]] virtual BindlessIndex GetBindlessIndex(BindlessKind kind) const noexcept = 0;
};

} // namespace Hylux::RHI
