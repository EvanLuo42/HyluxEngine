/// @file
/// @brief Splash-bindable progress reporter. Each bootstrap step writes a step name,
///        an integer percent, and an optional sub-message; QML observes the Q_PROPERTYs
///        via signal-driven binding.

#pragma once

#include <QObject>
#include <QString>

namespace Hylux::Editor
{

/// @brief Lives on the Qt main thread. Mutators are exposed to bootstrap code and are
///        safe to call from any thread (they marshal back to Qt main via QMetaObject).
class BootstrapProgress : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString stepName READ StepName NOTIFY changed)
    Q_PROPERTY(int     percent  READ Percent  NOTIFY changed)
    Q_PROPERTY(QString message  READ Message  NOTIFY changed)
public:
    explicit BootstrapProgress(QObject* parent = nullptr);
    ~BootstrapProgress() override = default;

    BootstrapProgress(const BootstrapProgress&)            = delete;
    BootstrapProgress& operator=(const BootstrapProgress&) = delete;

    [[nodiscard]] QString StepName() const { return stepName_; }
    [[nodiscard]] int     Percent() const { return percent_; }
    [[nodiscard]] QString Message() const { return message_; }

    void SetStep(QString stepName, int percent);
    void SetMessage(QString message);
    void SetPercent(int percent);

Q_SIGNALS:
    void changed();

private:
    QString stepName_;
    int     percent_ = 0;
    QString message_;
};

} // namespace Hylux::Editor
