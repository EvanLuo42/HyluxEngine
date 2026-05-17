/// @file
/// @brief Qt-main-thread dispatcher used to marshal callbacks from arbitrary worker
///        threads (most notably the engine game thread owned by EngineLoop) back onto
///        the Qt event loop. ViewModels chain Future continuations through here to
///        guarantee Q_PROPERTY mutations / signal emissions happen on the UI thread.

#pragma once

#include "Core/Async/Future.h"

#include <QObject>

#include <functional>
#include <utility>

namespace Hylux::Editor
{

/// @brief Lives on the Qt main thread. Post(fn) queues fn for execution on that thread
///        regardless of which thread invokes it; Then(future, cb) waits for the future
///        to resolve and then posts the continuation through Post.
class MainThreadDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit MainThreadDispatcher(QObject* parent = nullptr);
    ~MainThreadDispatcher() override = default;

    MainThreadDispatcher(const MainThreadDispatcher&)            = delete;
    MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;

    /// @brief Schedules fn for execution on the Qt main thread via a queued meta-call.
    ///        Safe to call from any thread.
    void Post(std::function<void()> fn);

    /// @brief Resolves a Future on whichever thread completes it, then posts the callback
    ///        through Post so it runs on the Qt main thread with the resolved value.
    template<class T> void Then(Future<T> future, std::function<void(T&)> onMain);
};

template<class T> void MainThreadDispatcher::Then(Future<T> future, std::function<void(T&)> onMain)
{
    future.Then([this, onMain = std::move(onMain)](T& value) mutable {
        Post([onMain = std::move(onMain), value]() mutable { onMain(value); });
    });
}

} // namespace Hylux::Editor
