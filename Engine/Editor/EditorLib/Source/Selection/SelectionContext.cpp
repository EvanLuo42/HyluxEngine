/// @file
/// @brief SelectionContext implementation.

#include "Selection/SelectionContext.h"

#include <algorithm>
#include <utility>

namespace Hylux::Editor
{

SelectionContext::SelectionContext(QObject* parent) : QObject(parent) {}

void SelectionContext::Select(Ref<ISelectable> selectable)
{
    if (!selectable)
    {
        Clear();
        return;
    }
    if (current_.size() == 1 && current_.front() && current_.front()->GetSelectionId() == selectable->GetSelectionId())
    {
        return;
    }
    current_.clear();
    current_.push_back(std::move(selectable));
    Q_EMIT selectionChanged();
}

void SelectionContext::AddToSelection(Ref<ISelectable> selectable)
{
    if (!selectable)
    {
        return;
    }
    const SelectionId id = selectable->GetSelectionId();
    for (const auto& existing : current_)
    {
        if (existing && existing->GetSelectionId() == id)
        {
            return;
        }
    }
    current_.push_back(std::move(selectable));
    Q_EMIT selectionChanged();
}

void SelectionContext::Deselect(SelectionId id)
{
    const auto before = current_.size();
    current_.erase(
        std::remove_if(current_.begin(), current_.end(),
                       [id](const Ref<ISelectable>& r) { return r && r->GetSelectionId() == id; }),
        current_.end());
    if (current_.size() != before)
    {
        Q_EMIT selectionChanged();
    }
}

void SelectionContext::Clear()
{
    if (current_.empty())
    {
        return;
    }
    current_.clear();
    Q_EMIT selectionChanged();
}

} // namespace Hylux::Editor
