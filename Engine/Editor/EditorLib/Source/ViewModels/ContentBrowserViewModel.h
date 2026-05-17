/// @file
/// @brief Placeholder ViewModel for the Content Browser dock. Real implementation will
///        enumerate the engine VFS and expose a path/list model; for now this is an
///        empty shell so the dock factory wires through.

#pragma once

#include "ViewModels/EditorViewModel.h"

namespace Hylux::Editor
{

class ContentBrowserViewModel : public EditorViewModel
{
    Q_OBJECT
public:
    explicit ContentBrowserViewModel(EditorContext& context, QObject* parent = nullptr);
    ~ContentBrowserViewModel() override = default;
};

} // namespace Hylux::Editor
