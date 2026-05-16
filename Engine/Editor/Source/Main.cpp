/// @file
/// @brief Entry point of the Hylux QML editor application.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("HyluxEditor");
    QGuiApplication::setOrganizationName("Hylux");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine qmlEngine;
    QObject::connect(
        &qmlEngine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    qmlEngine.loadFromModule("Hylux.Editor", "Main");

    const int rc = QGuiApplication::exec();
    return rc;
}
