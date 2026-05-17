/// @file
/// @brief Factory interface that produces one editor dock (title + QML view +
///        backing ViewModel). DockRegistry consumes a collection of these; DockHost
///        materialises ViewModels and exposes them to QML for binding.

#pragma once

#include <QObject>

#include <memory>
#include <string>

namespace Hylux::Editor
{

class EditorContext;

/// @brief Default position for a dock when the editor starts from a clean layout.
enum class DockArea
{
    Left,
    Right,
    Bottom,
    Center
};

/// @brief One factory per logical dock. The factory is stateless (or near-stateless)
///        and lives for the duration of the editor; the ViewModel it produces is
///        owned by DockHost.
class IDockFactory
{
public:
    virtual ~IDockFactory() = default;

    /// @brief Stable identifier used by KDDockWidgets' uniqueName and by ViewModel lookup.
    [[nodiscard]] virtual std::string Id() const = 0;

    /// @brief Display title shown on the dock tab and in the Window menu.
    [[nodiscard]] virtual std::string Title() const = 0;

    /// @brief QRC path (e.g. "qrc:/qt/qml/Hylux/Editor/Docks/LogDock.qml") loaded inside
    ///        the dock body. The QML accesses its ViewModel via dockHost.viewModel(id).
    [[nodiscard]] virtual std::string QmlSource() const = 0;

    /// @brief Default placement when no persisted layout is loaded.
    [[nodiscard]] virtual DockArea DefaultArea() const = 0;

    /// @brief Constructs the ViewModel. Called once during DockHost::Build; ownership
    ///        transfers to DockHost.
    [[nodiscard]] virtual std::unique_ptr<QObject> CreateViewModel(EditorContext& context) = 0;
};

} // namespace Hylux::Editor
