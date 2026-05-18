/// @file
/// @brief Editor application class: owns QGuiApplication, the engine instance, the
///        EngineLoop that drives it on a dedicated game thread, and the QML root.
///        Construction shows the splash and schedules ContinueBootstrap on the next
///        Qt event-loop tick; ContinueBootstrap runs the bootstrap steps, starts the
///        EngineLoop, builds the dock host and main window, then closes the splash.

#pragma once

#include "Engine/Engine.h"

#include <memory>

class QGuiApplication;

namespace Hylux::Editor
{
class BootstrapProgress;
class DockHost;
class DockRegistry;
class EditorContext;
class MainThreadDispatcher;
class MenuRegistry;
class QmlLogSink;
class SelectionContext;
} // namespace Hylux::Editor

namespace Hylux
{

class EngineLoop;
class SplashController;
class MainWindowController;

class EditorApp
{
public:
    EditorApp(int& argc, char** argv);
    ~EditorApp();

    EditorApp(const EditorApp&)            = delete;
    EditorApp& operator=(const EditorApp&) = delete;
    EditorApp(EditorApp&&)                 = delete;
    EditorApp& operator=(EditorApp&&)      = delete;

    /// @brief Enters the Qt event loop and returns its exit code.
    static int Run();

    [[nodiscard]] Engine& GetEngine() noexcept { return engine_; }

private:
    void ContinueBootstrap();
    void OnEngineInitialized(bool ok);
    void ShutdownLoopAndEngine();
    void RegisterBuiltinMenus() const;
    void RegisterEditorSubsystems();

    std::unique_ptr<QGuiApplication> guiApp_;
    Engine                           engine_;

    Editor::QmlLogSink* logSinkPtr_{nullptr};

    std::unique_ptr<SplashController>            splash_;
    std::unique_ptr<EngineLoop>                  engineLoop_;
    std::unique_ptr<Editor::MainThreadDispatcher> dispatcher_;
    std::unique_ptr<Editor::DockRegistry>        dockRegistry_;
    std::unique_ptr<Editor::DockHost>            dockHost_;
    std::unique_ptr<Editor::MenuRegistry>        menuRegistry_;
    std::unique_ptr<Editor::SelectionContext>    selectionContext_;
    std::unique_ptr<Editor::EditorContext>       editorContext_;
    std::unique_ptr<MainWindowController>        mainWindow_;
};

} // namespace Hylux
