/// @file
/// @brief Dedicated render-thread worker. Owns its own loop: the first BeginFrame is awaited
///        blocking; thereafter the loop non-blocking-drains structural mutations + the latest
///        BeginFrame, caches the most-recent scene snapshot, and re-renders it on every
///        iteration. Game-thread submission rate is therefore decoupled from render rate;
///        the renderer runs as fast as the GPU + slot ring allow.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHICommandPool.h"
#include "Renderer/Diagnostics/RendererStats.h"
#include "Renderer/RendererForward.h"
#include "Renderer/Thread/StructuralCommand.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace Hylux::RHI
{
class IRHIDevice;
class IRHIQueue;
class IRHIFence;
class IRHICommandList;
} // namespace Hylux::RHI

namespace Hylux::Renderer
{

class StructuralCommandQueue;
class FrameFenceTimeline;

/// @brief Owns the std::thread that drives every per-frame render submission. Construction
///        captures non-owning pointers to long-lived collaborators (device, queue, fence,
///        registries) which the subsystem guarantees outlive Start..Stop.
class RenderThread
{
public:
    /// @brief Non-owning collaborator handles. All pointers must remain valid for the
    ///        duration of Start..Stop.
    struct Deps
    {
        RHI::IRHIDevice*        device{nullptr};
        RHI::IRHIQueue*         queue{nullptr};
        RHI::IRHIFence*         frameFence{nullptr};
        StructuralCommandQueue* cmdQueue{nullptr};
        FrameFenceTimeline*     timeline{nullptr};
        ProxyRegistry*          proxies{nullptr};
        RenderResources*        resources{nullptr};
        TransformDoubleBuffer*  transformBuffer{nullptr};
        MaterialProxyCache*     materials{nullptr};
        PsoCache*               psoCache{nullptr};
        UploadHeapManager*      uploadHeap{nullptr};
    };

    explicit RenderThread(const Deps& deps) noexcept;
    ~RenderThread();

    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;

    /// @brief Spawns the worker thread. Returns false on failure (e.g. command pool create).
    bool Start();

    /// @brief Signals the worker to exit and blocks until it has joined. Idempotent.
    void Stop();

    [[nodiscard]] bool IsRunning() const noexcept { return started_.load(std::memory_order_acquire); }

    /// @brief Returns the most recent frame's stats. Safe to call from the game thread.
    [[nodiscard]] RendererStats SnapshotStats() const;

private:
    void Run();
    void DispatchMutation(const StructuralCommand& command);
    void RenderFrame(const BeginFrameCmd& begin, std::uint64_t renderFrameId,
                     RHI::IRHICommandList& cmd) const;

    Deps                                   deps_;
    Ref<RHI::IRHICommandPool>              commandPool_;
    std::uint64_t                          fenceValue_{0};
    std::uint64_t                          renderFrameId_{0};
    std::vector<Ref<RHI::IRHICommandList>> cmdListRing_;
    std::vector<std::uint64_t>             slotFenceValues_;
    BeginFrameCmd                          cachedBegin_{};
    bool                                   hasCachedBegin_{false};

    std::thread       thread_;
    std::atomic<bool> started_{false};

    mutable std::mutex statsMutex_;
    RendererStats      lastStats_{};
};

} // namespace Hylux::Renderer
