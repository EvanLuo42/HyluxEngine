/// @file
/// @brief SplashController implementation.

#include "Splash/SplashController.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScreen>
#include <QUrl>

namespace Hylux
{

SplashController::SplashController()
    : progress_(std::make_unique<Editor::BootstrapProgress>()), view_(std::make_unique<QQuickView>())
{
    view_->setFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    view_->setResizeMode(QQuickView::SizeRootObjectToView);
    view_->rootContext()->setContextProperty("bootstrap", progress_.get());
    view_->setSource(QUrl("qrc:/qt/qml/Hylux/Editor/Splash.qml"));
    view_->resize(520, 280);
    if (QScreen* screen = QGuiApplication::primaryScreen(); screen != nullptr)
    {
        const QRect geom = screen->availableGeometry();
        view_->setPosition(geom.center() - QPoint(view_->width() / 2, view_->height() / 2));
    }
}

SplashController::~SplashController() = default;

void SplashController::Show()
{
    view_->show();
}

void SplashController::Close()
{
    view_->close();
}

void SplashController::SetStatus(const QString& message)
{
    if (progress_)
    {
        progress_->SetStep(message, progress_->Percent());
    }
}

} // namespace Hylux
