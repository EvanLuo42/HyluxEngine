/// @file
/// @brief EditorViewModel implementation.

#include "ViewModels/EditorViewModel.h"

namespace Hylux::Editor
{

EditorViewModel::EditorViewModel(EditorContext& context, QObject* parent)
    : QObject(parent), context_(context)
{
}

} // namespace Hylux::Editor
