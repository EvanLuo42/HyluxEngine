/// @file
/// @brief Entry point of the Hylux QML editor application.

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/LogSystem.h"
#include "Core/Logging/Logger.h"
#include "Engine/Engine.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>

#include <chrono>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("HyluxEditor");
    QGuiApplication::setOrganizationName("Hylux");

    QQuickStyle::setStyle("Fusion");

    Hylux::Engine engine;
    engine.RegisterSubsystem<Hylux::LogSystem>(Hylux::LogSystemConfig{
        .async          = false,
        .enableConsole  = true,
        .enableFile     = true,
        .enableDebugger = true,
        .logDirectory   = "Logs",
    });
    engine.Initialize();

    HYLUX_LOG_INFO(Hylux::LogEngine, "HyluxEditor starting (Qt {}.{}.{})",
        QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&engine]()
    {
        HYLUX_LOG_INFO(Hylux::LogEngine, "HyluxEditor shutting down");
        engine.Shutdown();
    });

    auto lastTick = std::chrono::steady_clock::now();
    QTimer tickTimer;
    QObject::connect(&tickTimer, &QTimer::timeout, &app, [&engine, &lastTick]()
    {
        const auto  now   = std::chrono::steady_clock::now();
        const float delta = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;
        engine.Tick(delta);
    });
    tickTimer.start(16);

    QQmlApplicationEngine qmlEngine;
    QObject::connect(
        &qmlEngine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() {
            HYLUX_LOG_FATAL(Hylux::LogEngine, "QML root object failed to load");
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    qmlEngine.loadFromModule("Hylux.Editor", "Main");

    return QGuiApplication::exec();
}
