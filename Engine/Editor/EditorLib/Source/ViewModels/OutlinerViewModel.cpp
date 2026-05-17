/// @file
/// @brief OutlinerViewModel stub implementation.

#include "ViewModels/OutlinerViewModel.h"

#include "Context/EditorContext.h"

namespace Hylux::Editor
{

OutlinerViewModel::OutlinerViewModel(EditorContext& context, QObject* parent)
    : EditorViewModel(context, parent)
{
}

SelectionContext* OutlinerViewModel::Selection() noexcept
{
    return Context().Selection();
}

} // namespace Hylux::Editor
