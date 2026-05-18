/// @file
/// @brief Per-thread role stamping. Each major engine thread (game, render, worker) tags
///        itself on entry; other code can assert "I am being called on the right thread"
///        in Debug builds. Macros compile out in Release.

#pragma once

#include "Core/Utils/Assert.h"

namespace Hylux::Concurrency
{

/// @brief Logical role of a thread within the engine. Used only for affinity assertions;
///        the runtime does not enforce that any thread keeps its declared role.
enum class ThreadRole
{
    Unknown,
    GameThread,
    RenderThread,
    Worker,
};

/// @brief Tags the calling thread with @p role. Idempotent; later calls overwrite. The
///        owning thread of each role (EngineLoop for GameThread, RenderThread for
///        RenderThread, WorkerThreadPool for each Worker) registers exactly once on entry.
void RegisterCurrentThreadRole(ThreadRole role) noexcept;

/// @brief Returns the calling thread's role, or ThreadRole::Unknown if it never registered.
[[nodiscard]] ThreadRole GetCurrentThreadRole() noexcept;

/// @brief Returns the worker index of the calling thread within its pool, or -1 if the
///        thread is not a pool worker. Set alongside RegisterCurrentThreadRole(Worker)
///        by WorkerThreadPool; left at -1 for the game / render threads.
[[nodiscard]] int GetCurrentWorkerIndex() noexcept;

/// @brief Sets the per-thread worker index. Intended for use by WorkerThreadPool on worker
///        startup; not part of the public engine surface.
void SetCurrentWorkerIndex(int index) noexcept;

} // namespace Hylux::Concurrency

/// @brief Affinity assertion semantics: a thread without a registered role (Unknown) is
///        considered "opted out" — tests that drive subsystems directly and embedded hosts
///        that don't use EngineLoop pass freely. Once a role is registered, the assert is
///        strict. Net effect: catches worker/render-thread calls into game-thread-only API
///        without forcing every test or alternative host to register first.
#define HYLUX_ASSERT_GAME_THREAD()                                                                   \
    HYLUX_ASSERT_MSG(::Hylux::Concurrency::GetCurrentThreadRole() ==                                 \
                             ::Hylux::Concurrency::ThreadRole::GameThread ||                         \
                         ::Hylux::Concurrency::GetCurrentThreadRole() ==                             \
                             ::Hylux::Concurrency::ThreadRole::Unknown,                              \
                     "Expected to be on the game thread")

#define HYLUX_ASSERT_RENDER_THREAD()                                                                 \
    HYLUX_ASSERT_MSG(::Hylux::Concurrency::GetCurrentThreadRole() ==                                 \
                             ::Hylux::Concurrency::ThreadRole::RenderThread ||                       \
                         ::Hylux::Concurrency::GetCurrentThreadRole() ==                             \
                             ::Hylux::Concurrency::ThreadRole::Unknown,                              \
                     "Expected to be on the render thread")

#define HYLUX_ASSERT_WORKER_THREAD()                                                                 \
    HYLUX_ASSERT_MSG(::Hylux::Concurrency::GetCurrentThreadRole() ==                                 \
                             ::Hylux::Concurrency::ThreadRole::Worker ||                             \
                         ::Hylux::Concurrency::GetCurrentThreadRole() ==                             \
                             ::Hylux::Concurrency::ThreadRole::Unknown,                              \
                     "Expected to be on a worker thread")
