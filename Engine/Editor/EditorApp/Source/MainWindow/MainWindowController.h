/// @file
/// @brief Owns the editor's main QML window. Builds the dock ViewModels via
///        DockHost::Build, sets context properties for QML, and loads MainWindow.qml.

#pragma once

#include <memory>

class QQmlApplicationEngine;

namespace Hylux::Editor
{
class EditorContext;
}

namespace Hylux
{

class MainWindowController
{
public:
    explicit MainWindowController(Editor::EditorContext& context);
    ~MainWindowController();

    MainWindowController(const MainWindowController&)            = delete;
    MainWindowController& operator=(const MainWindowController&) = delete;

    void Load();
    void TearDown();

private:
    Editor::EditorContext&                 context_;
    std::unique_ptr<QQmlApplicationEngine> qmlEngine_;
};

} // namespace Hylux
