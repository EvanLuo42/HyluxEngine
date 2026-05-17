/// @file
/// @brief Owns the splash QQuickView and the BootstrapProgress object the splash QML
///        is bound to. EditorApp shows the splash before the first bootstrap step and
///        closes it once the main window is up.

#pragma once

#include "Bootstrap/BootstrapProgress.h"

#include <QString>

#include <memory>

class QQuickView;

namespace Hylux
{

class SplashController
{
public:
    SplashController();
    ~SplashController();

    SplashController(const SplashController&)            = delete;
    SplashController& operator=(const SplashController&) = delete;

    [[nodiscard]] Editor::BootstrapProgress* Progress() noexcept { return progress_.get(); }

    void Show();
    void Close();

    void SetStatus(const QString& message);

private:
    std::unique_ptr<Editor::BootstrapProgress> progress_;
    std::unique_ptr<QQuickView>                view_;
};

} // namespace Hylux
