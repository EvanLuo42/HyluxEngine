/// @file
/// @brief Engine subsystem that owns the runtime shader archive, the RHI shader-module
///        cache, and the global / per-material shader-map views. Editor builds open a
///        loose .hslib from the user's app-data directory; game builds open a cooked
///        archive shipped alongside the executable. Pak integration is intentionally
///        deferred and gated behind swapping the IBlobStore backend.

#pragma once

#include "Core/IO/Blob/IBlobStore.h"
#include "Engine/ISubsystem.h"
#include "Shader/ShaderMap/GlobalShaderMap.h"
#include "Shader/ShaderMap/MaterialShaderMap.h"
#include "Shader/ShaderMap/MaterialShaderMapCache.h"
#include "Shader/ShaderMap/ShaderModuleCache.h"
#include "Shader/ShaderRef.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hylux
{
class Engine;
class ITickable;
} // namespace Hylux

namespace Hylux::Shader
{

class IShaderArchive;

/// @brief Runtime entry point for shader lookups. Composes archive + cache + maps so the
///        renderer never deals with file IO or RHI module construction directly.
class ShaderSubsystem final : public ISubsystem
{
public:
    /// @brief Construction parameters. The subsystem takes ownership of the blob store.
    struct Config
    {
        std::unique_ptr<IBlobStore> blobStore;
        std::string                 archiveKey;
        bool                        enableHotReload{false};
        float                       hotReloadPollSeconds{1.0f};
    };

    using ArchiveReloadedCallback = std::function<void()>;

    explicit ShaderSubsystem(Config config);
    ~ShaderSubsystem() override;

    ShaderSubsystem(const ShaderSubsystem&)            = delete;
    ShaderSubsystem& operator=(const ShaderSubsystem&) = delete;
    ShaderSubsystem(ShaderSubsystem&&)                 = delete;
    ShaderSubsystem& operator=(ShaderSubsystem&&)      = delete;

    void Initialize(Engine& engine) override;
    void Shutdown() override;
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override;

    /// @brief Returns a pass shader by precomputed name hash. Use Shader::StablePassNameHash
    ///        (constexpr) at the call site to compute the hash once at compile time.
    [[nodiscard]] ShaderRef GetPassShader(std::uint64_t passNameHash,
                                          std::uint64_t permutationKey,
                                          RHI::ShaderStage stage);

    /// @brief Returns (and lazily constructs) the per-material shader-map for a given
    ///        material asset hash. The returned pointer is owned by the subsystem and is
    ///        invalidated on archive reload.
    [[nodiscard]] MaterialShaderMap* GetOrLoadMaterialShaderMap(std::uint64_t materialAssetHash);

    /// @brief Registers a callback invoked after a successful archive reload. Returns an
    ///        opaque id used to unsubscribe.
    std::size_t SubscribeArchiveReloaded(ArchiveReloadedCallback callback);

    /// @brief Removes a previously registered reload callback. Silent no-op on unknown id.
    void UnsubscribeArchiveReloaded(std::size_t id);

    /// @brief Re-opens the archive blob, rebuilds the cache and maps, and notifies every
    ///        reload subscriber. Called automatically by the hot-reload watcher (editor
    ///        only) and exposed for manual invocation in tooling.
    void ReloadAll();

    /// @brief Returns the active archive pointer (may be null before Initialize or after
    ///        a failed reload). Diagnostic accessor.
    [[nodiscard]] const IShaderArchive* GetArchive() const noexcept { return archive_.get(); }

    /// @brief Returns the active blob store (always non-null after construction).
    [[nodiscard]] IBlobStore* GetBlobStore() const noexcept { return config_.blobStore.get(); }

    /// @brief Returns the archive key currently being watched.
    [[nodiscard]] std::string_view GetArchiveKey() const noexcept { return config_.archiveKey; }

private:
    class HotReloadTickable;

    /// @brief Opens the configured archive and rebuilds the dependent objects. Returns
    ///        false if the blob could not be opened or parsed; logs the failure.
    bool OpenArchiveLocked();

    /// @brief Drops the archive and all dependent objects. Safe to call repeatedly.
    void DropArchiveLocked() noexcept;

    /// @brief Internal helper used by the hot-reload watcher.
    void OnHotReloadPoll();

    Config       config_;
    Engine*      engine_{nullptr};

    std::unique_ptr<IShaderArchive>         archive_;
    std::unique_ptr<ShaderModuleCache>      moduleCache_;
    std::unique_ptr<GlobalShaderMap>        globalShaderMap_;
    std::unique_ptr<MaterialShaderMapCache> materialMaps_;

    std::optional<std::filesystem::file_time_type> lastObservedTimestamp_{};

    std::unique_ptr<HotReloadTickable> hotReloadTickable_;

    std::size_t nextCallbackId_{1};
    std::unordered_map<std::size_t, ArchiveReloadedCallback> reloadCallbacks_;
};

} // namespace Hylux::Shader
