/// @file
/// @brief Central editor-wide selection store. Lives on the Qt main thread and is the
///        single source of truth for "what is selected"; Details / Outliner / Content
///        Browser docks observe selectionChanged and refresh from Current().

#pragma once

#include "Core/Memory/Ref.h"
#include "Selection/ISelectable.h"
#include "Selection/SelectionId.h"

#include <QObject>

#include <span>
#include <vector>

namespace Hylux::Editor
{

/// @brief Qt main-thread-only. Mutation methods coalesce into a single selectionChanged
///        emit per logical change so observers refresh exactly once.
class SelectionContext : public QObject
{
    Q_OBJECT
public:
    explicit SelectionContext(QObject* parent = nullptr);
    ~SelectionContext() override = default;

    /// @brief Replaces the current selection with a single entry. Emits selectionChanged
    ///        unless the new selection is identical to the previous one.
    void Select(Ref<ISelectable> selectable);

    /// @brief Appends to the selection. No-op (and no signal) when the id is already in
    ///        the set.
    void AddToSelection(Ref<ISelectable> selectable);

    /// @brief Removes a single entry by id. No-op when not present.
    void Deselect(SelectionId id);

    /// @brief Clears all entries. Emits selectionChanged when the set was non-empty.
    void Clear();

    [[nodiscard]] std::span<const Ref<ISelectable>> Current() const noexcept
    {
        return {current_.data(), current_.size()};
    }

    [[nodiscard]] std::size_t Size() const noexcept { return current_.size(); }
    [[nodiscard]] bool        IsEmpty() const noexcept { return current_.empty(); }

Q_SIGNALS:
    /// @brief Broadcast once after any mutation that actually changed the selection set.
    void selectionChanged();

private:
    std::vector<Ref<ISelectable>> current_;
};

} // namespace Hylux::Editor
