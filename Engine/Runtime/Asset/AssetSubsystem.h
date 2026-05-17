/// @file
/// @brief AssetSubsystem — the runtime owner of the asset registry, the LRU cache, and
///        the IAssetUploader used by Mesh / Texture loaders. Public Load<T> returns a
///        v2-shaped Future<Ref<T>>; v1 fulfills synchronously inside the call.

#pragma once

#include "Asset/AssetBase.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetTypeId.h"
#include "Core/Guid.h"
#include "Core/Async/Future.h"
#include "Core/Containers/RefLruCache.h"
#include "Core/Memory/Ref.h"
#include "Engine/ISubsystem.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <memory>
#include <string_view>
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

class AssetSubsystem final : public ISubsystem
{
public:
    explicit AssetSubsystem(AssetSubsystemConfig cfg);
    ~AssetSubsystem() override;

    AssetSubsystem(const AssetSubsystem&)            = delete;
    AssetSubsystem& operator=(const AssetSubsystem&) = delete;

    void                              Initialize(Engine& engine) override;
    void                              Shutdown() override;
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override;

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
    ///        matching per-type loader, returns the constructed asset. Called when the LRU
    ///        misses and the WeakRef cannot be resurrected.
    [[nodiscard]] Ref<AssetBase> ParseAndConstruct(const RegistryEntry& entry);

    AssetSubsystemConfig             config_;
    Engine*                          engine_{nullptr};
    IVirtualFileSystem*              vfs_{nullptr};
    RHI::RHISubsystem*               rhi_{nullptr};
    RHI::IRHIDevice*                 device_{nullptr};
    Shader::ShaderSubsystem*         shaders_{nullptr};
    Renderer::RenderSubsystem*       renderer_{nullptr};
    AssetRegistry                    registry_;
    RefLruCache<Guid, AssetBase>     lru_;
    std::unique_ptr<IAssetUploader>  uploader_;
    bool                             initialized_{false};
};

// ---------------------------------------------------------------------------
// Template definitions
// ---------------------------------------------------------------------------

template<typename T>
Future<Ref<T>> AssetSubsystem::Load(std::string_view virtualPath)
{
    auto            generic   = LoadGenericByPath(virtualPath);
    Ref<AssetBase>& baseRef   = generic.Wait();
    if (!baseRef || baseRef->GetAssetTypeId() != T::kTypeId)
    {
        return Future<Ref<T>>::MakeFailed();
    }
    Ref<T> typed(static_cast<T*>(baseRef.Get()));
    return Future<Ref<T>>::MakeReady(std::move(typed));
}

template<typename T>
Future<Ref<T>> AssetSubsystem::LoadByGuid(Guid guid)
{
    auto            generic = LoadGenericByGuid(guid);
    Ref<AssetBase>& baseRef = generic.Wait();
    if (!baseRef || baseRef->GetAssetTypeId() != T::kTypeId)
    {
        return Future<Ref<T>>::MakeFailed();
    }
    Ref<T> typed(static_cast<T*>(baseRef.Get()));
    return Future<Ref<T>>::MakeReady(std::move(typed));
}

} // namespace Hylux::Asset
