/// @file
/// @brief Placeholder ViewModel for the Details dock. Reflects the current
///        SelectionContext into a simple QStringList of display names so the QML can
///        at least show that selection is wired end-to-end.

#pragma once

#include "ViewModels/EditorViewModel.h"

#include <QStringList>

namespace Hylux::Editor
{

class DetailsViewModel : public EditorViewModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectionNames READ SelectionNames NOTIFY selectionNamesChanged)
public:
    explicit DetailsViewModel(EditorContext& context, QObject* parent = nullptr);
    ~DetailsViewModel() override = default;

    [[nodiscard]] QStringList SelectionNames() const noexcept { return selectionNames_; }

Q_SIGNALS:
    void selectionNamesChanged();

private Q_SLOTS:
    void RebuildFromSelection();

private:
    QStringList selectionNames_;
};

} // namespace Hylux::Editor
