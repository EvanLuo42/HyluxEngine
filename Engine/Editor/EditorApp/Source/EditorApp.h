/// @file
/// @brief Editor application class: owns QGuiApplication, the Hylux Engine, and the QML root.

#pragma once

#include "Engine/Engine.h"

#include <chrono>
#include <memory>

class QGuiApplication;
class QQmlApplicationEngine;
class QTimer;

namespace Hylux
{

/// @brief Top-level editor lifetime holder. Constructed once in main(), Run() pumps the Qt loop.
///        Engine subsystems initialize during construction; Shutdown is wired to aboutToQuit so
///        log lines from QObject destructors still flow through the configured sinks.
class EditorApp
{
public:
    EditorApp(int& argc, char** argv);
    ~EditorApp();
    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;
    EditorApp(EditorApp&&) = delete;
    EditorApp& operator=(EditorApp&&) = delete;

    /// @brief Enters the Qt event loop and returns its exit code.
    static int Run();

    [[nodiscard]] Engine& GetEngine() noexcept { return engine_; }

private:
    std::unique_ptr<QGuiApplication>       guiApp_;
    Engine                                 engine_;
    std::unique_ptr<QTimer>                tickTimer_;
    std::unique_ptr<QQmlApplicationEngine> qmlEngine_;
    std::chrono::steady_clock::time_point  lastTick_;
};

} // namespace Hylux
