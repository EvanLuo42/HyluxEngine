/// @file
/// @brief Placeholder ViewModel for the world Outliner dock. The concrete tree model
///        backed by the engine's entity store lands in a follow-up task; for now this
///        exposes only the selection context so the QML can react to selectionChanged.

#pragma once

#include "ViewModels/EditorViewModel.h"

namespace Hylux::Editor
{

class SelectionContext;

class OutlinerViewModel : public EditorViewModel
{
    Q_OBJECT
    Q_PROPERTY(Hylux::Editor::SelectionContext* selection READ Selection CONSTANT)
public:
    explicit OutlinerViewModel(EditorContext& context, QObject* parent = nullptr);
    ~OutlinerViewModel() override = default;

    [[nodiscard]] SelectionContext* Selection() noexcept;
};

} // namespace Hylux::Editor
