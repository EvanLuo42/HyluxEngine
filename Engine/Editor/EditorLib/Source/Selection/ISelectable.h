/// @file
/// @brief Adapter interface every selectable object exposes to the editor. Concrete
///        adapters (EntitySelectable, AssetSelectable, MaterialSelectable…) wrap a
///        runtime handle and surface it as a SelectionContext entry. Adapters are
///        refcounted via the engine's intrusive Ref<>; SelectionContext holds Ref<>
///        so a selectable stays alive at least as long as it is selected.

#pragma once

#include "Core/Memory/RefCounted.h"
#include "Selection/SelectionId.h"

#include <string_view>

namespace Hylux::Editor
{

/// @brief Editor-side handle to something the user can select. Lifetime is governed by
///        the SelectionContext's Ref to it.
class ISelectable : public RefCounted
{
public:
    ~ISelectable() override = default;

    /// @brief Stable identity used for deduplication, removal from selection, and
    ///        cross-dock subscriptions.
    [[nodiscard]] virtual SelectionId GetSelectionId() const = 0;

    /// @brief Short text label rendered in Outliner / Details headers. UTF-8.
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
};

} // namespace Hylux::Editor
