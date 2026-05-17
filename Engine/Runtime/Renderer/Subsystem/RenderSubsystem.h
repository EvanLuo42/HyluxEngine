/// @file
/// @brief Engine subsystem that owns the render pipeline. Stage 3 introduces a dedicated
///        render thread: SubmitFrame on the game thread enqueues a BeginFrame command
///        through StructuralCommandQueue and acquires a slot on FrameFenceTimeline; the
///        render thread drains the queue, builds graphs, and submits to the GPU. Structural
///        mutations (spawn / despawn / material / mesh / flags) go through the same queue.

#pragma once

#include "Core/Memory/Ref.h"
#include "Engine/ISubsystem.h"
#include "RHI/IRHIFence.h"
#include "RHI/IRHIPipelineCache.h"
#include "RHI/IRHIQueue.h"
#include "RHI/RHIForward.h"
#include "Renderer/Proxy/ProxyId.h"
#include "Renderer/RendererForward.h"
#include "Renderer/Subsystem/RendererConfig.h"
#include "Renderer/Subsystem/RendererFrame.h"
#include "Renderer/Thread/TransformDoubleBuffer.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Hylux
{
class Engine;
}

namespace Hylux::RHI
{
class RHISubsystem;
}

namespace Hylux::Shader
{
class ShaderSubsystem;
}

namespace Hylux::Asset
{
class MeshAsset;
class MaterialInstance;
} // namespace Hylux::Asset

namespace Hylux::Renderer
{

class ProxyRegistry;
class RenderResources;
struct SceneViewRequest;

/// @brief Spawn-time payload. When `mesh` is non-null SpawnPrimitive auto-enqueues an
///        AssignMesh command for the mesh's stable handle plus a SetBounds command using
///        the mesh's local AABB; callers may still override bounds afterwards with
///        SetPrimitiveBounds.
struct PrimitiveSpawnDesc
{
    std::uint32_t            layerMask{0xFFFFFFFFu};
    const Asset::MeshAsset*  mesh{nullptr};
};

/// @brief Top-level renderer subsystem. All public methods must be called from the same
///        thread that owns the Engine instance (the game / main thread).
class RenderSubsystem final : public ISubsystem
{
public:
    explicit RenderSubsystem(RendererConfig config);
    ~RenderSubsystem() override;

    RenderSubsystem(const RenderSubsystem&) = delete;
    RenderSubsystem& operator=(const RenderSubsystem&) = delete;
    RenderSubsystem(RenderSubsystem&&) = delete;
    RenderSubsystem& operator=(RenderSubsystem&&) = delete;

    void Initialize(Engine& engine) override;
    void Shutdown() override;
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override;

    /// @brief Allocates a new proxy id (atomic, game-thread safe) and enqueues a spawn
    ///        command for the render thread. Stage 3: registry application is a no-op.
    [[nodiscard]] ProxyId SpawnPrimitive(const PrimitiveSpawnDesc& desc = {}) const;

    /// @brief Enqueues a despawn command. The id is logically valid until the render
    ///        thread retires the command in the next frame.
    void DespawnPrimitive(ProxyId id) const;

    /// @brief Enqueues a material rebinding. Snapshots the instance on the calling
    ///        thread and ships the bytes across the structural command queue; the render
    ///        thread resolves them through MaterialProxyCache.
    void AssignMaterial(ProxyId id, const MaterialInstance& instance) const;

    /// @brief Enqueues a mesh rebinding. Stage 4 wires this through MeshProxy storage.
    void AssignMesh(ProxyId id, std::uint64_t meshHandle) const;

    /// @brief Enqueues a layer / visibility / render-queue flag update.
    void SetPrimitiveFlags(ProxyId id, std::uint32_t flags) const;

    /// @brief Enqueues a local-space AABB update used by frustum / occlusion culling.
    void SetPrimitiveBounds(ProxyId id, const PrimitiveBounds& bounds) const;

    /// @brief Writes the latest transform into the writing half of TransformDoubleBuffer.
    ///        Game-thread safe across multiple callers as long as each targets a distinct id.
    void UpdateTransform(ProxyId id, const TransformMat3x4& matrix, std::uint32_t flags = 0) const;

    /// @brief Acquires a frame slot, enqueues a BeginFrame command with the view list,
    ///        and returns immediately. Blocks only when framesInFlight frames are already
    ///        pending render-thread processing.
    void SubmitFrame(std::span<const SceneViewRequest> views);

    /// @brief Blocks until (a) every game-thread submission currently in the queue has been
    ///        consumed by the self-looping render thread and (b) every GPU command list
    ///        the render thread has submitted so far has signalled its fence. After this
    ///        returns the host may destroy any resource previously referenced by a frame
    ///        it submitted, without racing the render thread's vkQueueSubmit calls (only
    ///        the timeline semaphore and the frame fence are touched). The render thread
    ///        continues looping over its now-current cached scene.
    void WaitForFramesConsumed() const;

    /// @brief Full shutdown drain: stops the render thread (joining it) and then waits for
    ///        the GPU queue and device to become idle. Only safe to call from Shutdown —
    ///        the renderer cannot be used afterwards.
    void FlushAndWaitIdle();

    /// @brief Hands ownership of a render path to the subsystem. Returns a non-owning
    ///        pointer the host attaches to SceneViewRequest::path.
    [[nodiscard]] IRenderPath* RegisterRenderPath(std::unique_ptr<IRenderPath> path);

    [[nodiscard]] RendererStats GetLastFrameStats() const noexcept;
    [[nodiscard]] std::uint64_t GetFrameId() const noexcept { return frame_.GetFrameId(); }
    [[nodiscard]] RHI::IRHIDevice* GetDevice() const noexcept { return device_; }
    [[nodiscard]] PsoCache* GetPsoCache() const noexcept { return psoCache_.get(); }
    [[nodiscard]] FrameFenceTimeline* GetTimeline() const noexcept { return timeline_.get(); }
    [[nodiscard]] StructuralCommandQueue* GetCommandQueue() const noexcept { return cmdQueue_.get(); }
    [[nodiscard]] TransformDoubleBuffer* GetTransformBuffer() const noexcept { return transformBuffer_.get(); }
    [[nodiscard]] MaterialProxyCache* GetMaterialCache() const noexcept { return materials_.get(); }
    [[nodiscard]] UploadHeapManager* GetUploadHeap() const noexcept { return uploadHeap_.get(); }

private:
    RendererConfig config_;
    Engine* engine_{nullptr};
    RHI::RHISubsystem* rhi_{nullptr};
    Shader::ShaderSubsystem* shaders_{nullptr};
    RHI::IRHIDevice* device_{nullptr};

    Ref<RHI::IRHIQueue> graphicsQueue_;
    Ref<RHI::IRHIFence> frameFence_;
    Ref<RHI::IRHIPipelineCache> backendPsoCache_;

    std::unique_ptr<ProxyRegistry> proxies_;
    std::unique_ptr<RenderResources> resources_;
    std::unique_ptr<PsoBlobStore> psoBlobStore_;
    std::unique_ptr<PipelineLayoutCache> layoutCache_;
    std::unique_ptr<PsoCache> psoCache_;
    std::unique_ptr<TransformDoubleBuffer> transformBuffer_;
    std::unique_ptr<UploadHeapManager> uploadHeap_;
    std::unique_ptr<MaterialProxyCache> materials_;
    std::unique_ptr<FrameFenceTimeline> timeline_;
    std::unique_ptr<StructuralCommandQueue> cmdQueue_;
    std::unique_ptr<RenderThread> renderThread_;
    std::vector<std::unique_ptr<IRenderPath>> renderPaths_;

    RendererFrame frame_;
    std::size_t shaderReloadCallbackId_{0};
    bool initialized_{false};
};

} // namespace Hylux::Renderer
