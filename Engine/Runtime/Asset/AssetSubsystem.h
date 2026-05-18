/// @file
/// @brief AssetSubsystem — the runtime owner of the asset registry, the LRU cache, and
///        the async IAssetUploader used by Mesh / Texture loaders. Public Load<T>
///        returns Future<Ref<T>> that resolves once the asset is fully constructed and
///        any GPU upload has completed. An in-flight map dedups concurrent loads of the
///        same Guid so callers always share the same Future. The LRU + in-flight map
///        are guarded by stateMutex_ because continuations may fire from the uploader's
///        worker thread.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetTypeId.h"
#include "Core/Async/Future.h"
#include "Core/Async/IExecutor.h"
#include "Core/Containers/RefLruCache.h"
#include "Core/Guid.h"
#include "Core/Memory/Ref.h"
#include "Engine/ISubsystem.h"
#include "Engine/ITickable.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Hylux
{
class Engine;
class IVirtualFileSystem;
} // namespace Hylux

namespace Hylux::RHI
{
class RHISubsystem;
} // namespace Hylux::RHI

namespace Hylux::Shader
{
class ShaderSubsystem;
} // namespace Hylux::Shader

namespace Hylux::Renderer
{
class RenderSubsystem;
} // namespace Hylux::Renderer

namespace Hylux::Asset
{

class IAssetUploader;

struct AssetSubsystemConfig
{
    /// @brief Soft byte cap for the LRU. External Ref<> consumers keep their values alive
    ///        past eviction, so the budget is a hint at how much GPU-resident asset data
    ///        the cache attempts to keep loaded.
    std::uint64_t lruBudgetBytes{512ull * 1024 * 1024};

    /// @brief Whether the subsystem walks the /Engine/ and /Game/ mount roots during
    ///        Initialize to populate the registry. Disable in unit-test contexts where the
    ///        host wants to control the registry contents.
    bool scanEngineMountAtInit{true};
    bool scanGameMountAtInit{true};

    /// @brief Reserved bytes per upload staging operation. Used as a default minimum
    ///        allocation so small uploads don't churn the underlying allocator.
    std::uint64_t uploadStagingChunkBytes{4ull * 1024 * 1024};
};

class AssetSubsystem final : public ISubsystem, public ITickable
{
public:
    explicit AssetSubsystem(AssetSubsystemConfig cfg);
    ~AssetSubsystem() override;

    AssetSubsystem(const AssetSubsystem&)            = delete;
    AssetSubsystem& operator=(const AssetSubsystem&) = delete;

    void                              Initialize(Engine& engine) override;
    void                              Shutdown() override;
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override;

    /// @brief Drains the game-thread executor every frame. Game-thread executor is
    ///        the marshaling target consumers should explicitly opt into via
    ///        `.Then(assets->GetGameThreadExecutor(), cb)` when they need the
    ///        continuation to land on game thread (e.g. touching scene state).
    ///        Loader internals stay inline so `.Wait()` on game thread does not
    ///        deadlock against the tick.
    void Tick(float deltaSeconds) override;

    /// @brief Early tick order — game-thread continuations posted by consumers need to
    ///        run before the systems that depend on the resolved asset state later in
    ///        the same frame (renderer, gameplay).
    [[nodiscard]] int TickOrder() const override { return -100; }

    /// @brief Game-thread executor drained inside Tick. Pass this to Future::Then /
    ///        Future::Transform when you need the continuation to run on the game
    ///        thread rather than on whichever thread fulfilled the producer Promise
    ///        (typically the upload worker for asset Futures).
    [[nodiscard]] IExecutor& GetGameThreadExecutor() noexcept { return tickExecutor_; }

    /// @brief Loads by VFS path. Type T must publish a `static constexpr AssetTypeId kTypeId`
    ///        matching the cooked file's typeTag; a mismatch produces a failed future.
    template<typename T>
    [[nodiscard]] Future<Ref<T>> Load(std::string_view virtualPath);

    template<typename T>
    [[nodiscard]] Future<Ref<T>> LoadByGuid(Guid guid);

    /// @brief Type-erased load by GUID. Used internally and by per-type loaders that need
    ///        to recurse for dependent assets (e.g. MaterialInstanceLoader → parent material).
    [[nodiscard]] Future<Ref<AssetBase>> LoadGenericByGuid(Guid guid);

    [[nodiscard]] const AssetRegistry& GetRegistry() const noexcept { return registry_; }

    void                          UnloadAll();
    [[nodiscard]] std::uint64_t   GetLruBytesInUse() const noexcept { return lru_.BytesInUse(); }
    [[nodiscard]] std::size_t     GetLruEntryCount() const noexcept { return lru_.Size(); }

private:
    [[nodiscard]] Future<Ref<AssetBase>> LoadGenericByPath(std::string_view virtualPath);

    /// @brief Cold path: opens the .hass via CookedReader, dispatches by typeTag to the
    ///        matching per-type loader, returns a Future that resolves to the constructed
    ///        asset once any GPU upload behind it completes. Called when the LRU misses
    ///        and the WeakRef cannot be resurrected.
    [[nodiscard]] Future<Ref<AssetBase>> ParseAndConstruct(const RegistryEntry& entry);

    AssetSubsystemConfig                                     config_;
    Engine*                                                  engine_{nullptr};
    IVirtualFileSystem*                                      vfs_{nullptr};
    RHI::RHISubsystem*                                       rhi_{nullptr};
    RHI::IRHIDevice*                                         device_{nullptr};
    Shader::ShaderSubsystem*                                 shaders_{nullptr};
    Renderer::RenderSubsystem*                               renderer_{nullptr};
    AssetRegistry                                            registry_;

    mutable std::mutex                               stateMutex_;
    RefLruCache<Guid, AssetBase>                     lru_;
    std::unordered_map<Guid, Future<Ref<AssetBase>>> inflight_;

    QueuedExecutor                                   tickExecutor_;
    std::unique_ptr<IAssetUploader>                  uploader_;
    bool                                             initialized_{false};
};

// ---------------------------------------------------------------------------
// Template definitions
// ---------------------------------------------------------------------------

template<typename T>
Future<Ref<T>> AssetSubsystem::Load(std::string_view virtualPath)
{
    auto generic = LoadGenericByPath(virtualPath);
    if (!generic.IsValid())
    {
        return Future<Ref<T>>::MakeFailed();
    }
    return generic.template Transform<Ref<T>>([](Ref<AssetBase>& base) -> Ref<T> {
        if (!base || base->GetAssetTypeId() != T::kTypeId)
        {
            return {};
        }
        return Ref<T>(static_cast<T*>(base.Get()));
    });
}

template<typename T>
Future<Ref<T>> AssetSubsystem::LoadByGuid(Guid guid)
{
    auto generic = LoadGenericByGuid(guid);
    if (!generic.IsValid())
    {
        return Future<Ref<T>>::MakeFailed();
    }
    return generic.template Transform<Ref<T>>([](Ref<AssetBase>& base) -> Ref<T> {
        if (!base || base->GetAssetTypeId() != T::kTypeId)
        {
            return {};
        }
        return Ref<T>(static_cast<T*>(base.Get()));
    });
}

} // namespace Hylux::Asset
