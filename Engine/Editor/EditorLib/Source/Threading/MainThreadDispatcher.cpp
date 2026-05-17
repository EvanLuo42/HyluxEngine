/// @file
/// @brief MainThreadDispatcher implementation: thin wrapper around
///        QMetaObject::invokeMethod with Qt::QueuedConnection.

#include "Threading/MainThreadDispatcher.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

#include <utility>

namespace Hylux::Editor
{

MainThreadDispatcher::MainThreadDispatcher(QObject* parent) : QObject(parent)
{
    if (QCoreApplication* app = QCoreApplication::instance(); app != nullptr)
    {
        moveToThread(app->thread());
    }
}

void MainThreadDispatcher::Post(std::function<void()> fn)
{
    if (!fn)
    {
        return;
    }
    QMetaObject::invokeMethod(
        this, [fn = std::move(fn)]() mutable { fn(); }, Qt::QueuedConnection);
}

} // namespace Hylux::Editor
