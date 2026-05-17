/// @file
/// @brief DetailsViewModel implementation.

#include "ViewModels/DetailsViewModel.h"

#include "Context/EditorContext.h"
#include "Selection/SelectionContext.h"

namespace Hylux::Editor
{

DetailsViewModel::DetailsViewModel(EditorContext& context, QObject* parent)
    : EditorViewModel(context, parent)
{
    if (SelectionContext* selection = Context().Selection(); selection != nullptr)
    {
        connect(selection, &SelectionContext::selectionChanged, this, &DetailsViewModel::RebuildFromSelection);
        RebuildFromSelection();
    }
}

void DetailsViewModel::RebuildFromSelection()
{
    QStringList names;
    if (SelectionContext* selection = Context().Selection(); selection != nullptr)
    {
        const auto current = selection->Current();
        names.reserve(static_cast<int>(current.size()));
        for (const auto& item : current)
        {
            if (item)
            {
                names.append(QString::fromUtf8(item->GetDisplayName().data(),
                                                static_cast<int>(item->GetDisplayName().size())));
            }
        }
    }
    if (names != selectionNames_)
    {
        selectionNames_ = names;
        Q_EMIT selectionNamesChanged();
    }
}

} // namespace Hylux::Editor
