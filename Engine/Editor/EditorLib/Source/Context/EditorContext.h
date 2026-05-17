/// @file
/// @brief Editor-wide service locator exposed to QML as a context property. ViewModels
///        accept an EditorContext& in their constructor and use it to access selection,
///        the engine command queue, the dispatcher, and the dock / menu registries.
///        Owned by EditorApp; lifetime spans from main window creation to shutdown.

#pragma once

#include "Core/Async/Future.h"
#include "Dock/DockHost.h"
#include "Engine/EngineLoop.h"
#include "Menu/MenuRegistry.h"
#include "Selection/SelectionContext.h"
#include "Threading/MainThreadDispatcher.h"

#include <QObject>

#include <functional>
#include <type_traits>
#include <utility>

namespace Hylux
{
class Engine;
} // namespace Hylux

namespace Hylux::Editor
{

class DockRegistry;

/// @brief Aggregates the editor-side singletons. Pointers stay valid for the lifetime
///        of the EditorContext; ViewModels can cache them. ViewModels submit engine
///        work through Submit / Invoke and never touch Engine directly.
class EditorContext : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Hylux::Editor::SelectionContext* selection READ Selection CONSTANT)
    Q_PROPERTY(Hylux::Editor::MenuRegistry* menus READ Menus CONSTANT)
    Q_PROPERTY(Hylux::Editor::DockHost* docks READ Docks CONSTANT)
public:
    EditorContext(EngineLoop&           loop,
                  MainThreadDispatcher& dispatcher,
                  DockRegistry&         dockRegistry,
                  DockHost&             dockHost,
                  MenuRegistry&         menuRegistry,
                  SelectionContext&     selection,
                  QObject*              parent = nullptr);
    ~EditorContext() override = default;

    EditorContext(const EditorContext&)            = delete;
    EditorContext& operator=(const EditorContext&) = delete;

    [[nodiscard]] EngineLoop&            Loop() noexcept { return loop_; }
    [[nodiscard]] MainThreadDispatcher&  Dispatcher() noexcept { return dispatcher_; }
    [[nodiscard]] DockRegistry&          DockRegistryRef() noexcept { return dockRegistry_; }
    [[nodiscard]] DockHost*              Docks() noexcept { return &dockHost_; }
    [[nodiscard]] MenuRegistry*          Menus() noexcept { return &menuRegistry_; }
    [[nodiscard]] SelectionContext*      Selection() noexcept { return &selection_; }

    /// @brief Fire-and-forget engine submission. Closure runs on the game thread.
    void Submit(std::function<void(Engine&)> cmd);

    /// @brief Engine submission with a future result. Non-void return required.
    template<class F> auto Invoke(F&& fn);

private:
    EngineLoop&           loop_;
    MainThreadDispatcher& dispatcher_;
    DockRegistry&         dockRegistry_;
    DockHost&             dockHost_;
    MenuRegistry&         menuRegistry_;
    SelectionContext&     selection_;
};

template<class F> auto EditorContext::Invoke(F&& fn)
{
    using R = std::invoke_result_t<F, Engine&>;
    static_assert(!std::is_void_v<R>, "EditorContext::Invoke requires non-void return; use Submit for void");
    return loop_.Invoke(std::forward<F>(fn));
}

} // namespace Hylux::Editor
