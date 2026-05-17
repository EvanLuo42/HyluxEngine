/// @file
/// @brief MainWindowController implementation.

#include "MainWindow/MainWindowController.h"

#include "Context/EditorContext.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Dock/DockHost.h"
#include "Menu/MenuRegistry.h"
#include "Selection/SelectionContext.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <kddockwidgets/KDDockWidgets.h>
#include <kddockwidgets/qtquick/Platform.h>

namespace Hylux
{

MainWindowController::MainWindowController(Editor::EditorContext& context) : context_(context) {}

MainWindowController::~MainWindowController()
{
    TearDown();
}

void MainWindowController::Load()
{
    KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtQuick);

    qmlEngine_ = std::make_unique<QQmlApplicationEngine>();
    if (auto* platform = KDDockWidgets::QtQuick::Platform::instance(); platform != nullptr)
    {
        platform->setQmlEngine(qmlEngine_.get());
    }

    QQmlContext* root = qmlEngine_->rootContext();
    root->setContextProperty("editorContext", &context_);
    root->setContextProperty("dockHost", context_.Docks());
    root->setContextProperty("menuRegistry", context_.Menus());
    root->setContextProperty("selectionContext", context_.Selection());

    QObject::connect(qmlEngine_.get(), &QQmlApplicationEngine::objectCreationFailed, qmlEngine_.get(),
                     []() { HYLUX_LOG_FATAL(LogEngine, "MainWindow QML failed to load"); });

    qmlEngine_->loadFromModule("Hylux.Editor", "MainWindow");
}

void MainWindowController::TearDown()
{
    qmlEngine_.reset();
}

} // namespace Hylux
