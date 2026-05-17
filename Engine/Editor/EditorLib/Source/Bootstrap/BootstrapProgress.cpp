/// @file
/// @brief BootstrapProgress implementation: mutators marshal to Qt main via
///        QMetaObject::invokeMethod so worker-thread callers stay safe.

#include "Bootstrap/BootstrapProgress.h"

#include <QMetaObject>
#include <QThread>

#include <algorithm>
#include <utility>

namespace Hylux::Editor
{

BootstrapProgress::BootstrapProgress(QObject* parent) : QObject(parent) {}

void BootstrapProgress::SetStep(QString stepName, int percent)
{
    if (thread() != QThread::currentThread())
    {
        QMetaObject::invokeMethod(
            this, [this, stepName = std::move(stepName), percent]() mutable {
                SetStep(std::move(stepName), percent);
            }, Qt::QueuedConnection);
        return;
    }
    stepName_ = std::move(stepName);
    percent_  = std::clamp(percent, 0, 100);
    Q_EMIT changed();
}

void BootstrapProgress::SetMessage(QString message)
{
    if (thread() != QThread::currentThread())
    {
        QMetaObject::invokeMethod(
            this, [this, message = std::move(message)]() mutable { SetMessage(std::move(message)); },
            Qt::QueuedConnection);
        return;
    }
    message_ = std::move(message);
    Q_EMIT changed();
}

void BootstrapProgress::SetPercent(int percent)
{
    if (thread() != QThread::currentThread())
    {
        QMetaObject::invokeMethod(this, [this, percent]() { SetPercent(percent); }, Qt::QueuedConnection);
        return;
    }
    percent_ = std::clamp(percent, 0, 100);
    Q_EMIT changed();
}

} // namespace Hylux::Editor
