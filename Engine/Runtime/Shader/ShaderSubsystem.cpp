/// @file
/// @brief ShaderSubsystem implementation. Lifetime: Engine::Initialize calls Initialize
///        which opens the archive and (optionally) registers a tick-driven file watcher.
///        Shutdown tears everything down in the reverse order, leaving the RHI device
///        untouched. Hot reload is an editor-only convenience driven by polling the
///        blob store's LastModified.

#include "Shader/ShaderSubsystem.h"

#include "Shader/Diagnostics/ShaderLogCategories.h"
#include "Shader/ShaderArchive/MappedShaderArchive.h"

#include "Core/Logging/Logger.h"
#include "Engine/Engine.h"
#include "Engine/ITickable.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHISubsystem.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace Hylux::Shader
{

class ShaderSubsystem::HotReloadTickable final : public ITickable
{
public:
    HotReloadTickable(ShaderSubsystem& owner, float pollSeconds) noexcept
        : owner_(owner), pollInterval_(std::max(pollSeconds, 0.05f))
    {
    }

    void Tick(float deltaSeconds) override
    {
        accumulator_ += deltaSeconds;
        if (accumulator_ < pollInterval_)
        {
            return;
        }
        accumulator_ = 0.0f;
        owner_.OnHotReloadPoll();
    }

    [[nodiscard]] int TickOrder() const override { return -1000; }

private:
    ShaderSubsystem& owner_;
    float            pollInterval_;
    float            accumulator_{0.0f};
};

ShaderSubsystem::ShaderSubsystem(Config config)
    : config_(std::move(config))
{
    assert(config_.blobStore != nullptr && "ShaderSubsystem requires a non-null blob store");
    assert(!config_.archiveKey.empty() && "ShaderSubsystem requires a non-empty archive key");
}

ShaderSubsystem::~ShaderSubsystem() = default;

std::vector<TypeId> ShaderSubsystem::GetDependencies() const
{
    return { TypeIdOf<RHI::RHISubsystem>() };
}

void ShaderSubsystem::Initialize(Engine& engine)
{
    engine_ = &engine;

    auto* rhi = engine.GetSubsystem<RHI::RHISubsystem>();
    if (rhi == nullptr || rhi->GetDevice() == nullptr)
    {
        HYLUX_LOG_ERROR(LogShader,
                        "ShaderSubsystem::Initialize: RHISubsystem unavailable; shader lookups "
                        "will return empty refs");
    }

    if (!OpenArchiveLocked())
    {
        HYLUX_LOG_WARN(LogShader,
                       "ShaderSubsystem::Initialize: archive '{}' not opened; shader lookups "
                       "will return empty refs until ReloadAll succeeds",
                       config_.archiveKey);
    }

    if (config_.enableHotReload)
    {
        hotReloadTickable_ =
            std::make_unique<HotReloadTickable>(*this, config_.hotReloadPollSeconds);
        engine_->RegisterTickable(hotReloadTickable_.get());
        HYLUX_LOG_INFO(LogShader,
                       "ShaderSubsystem: hot-reload watcher armed (poll every {:.2f}s) on {}",
                       config_.hotReloadPollSeconds, config_.archiveKey);
    }
}

void ShaderSubsystem::Shutdown()
{
    if (engine_ != nullptr && hotReloadTickable_ != nullptr)
    {
        engine_->UnregisterTickable(hotReloadTickable_.get());
    }
    hotReloadTickable_.reset();

    reloadCallbacks_.clear();
    DropArchiveLocked();
    engine_ = nullptr;
}

bool ShaderSubsystem::OpenArchiveLocked()
{
    auto blob = config_.blobStore->OpenMapped(config_.archiveKey);
    if (blob == nullptr)
    {
        HYLUX_LOG_WARN(LogShader,
                       "ShaderSubsystem::OpenArchive: blob store {} could not open '{}'",
                       config_.blobStore->DebugName(), config_.archiveKey);
        return false;
    }

    auto archive = MappedShaderArchive::Open(std::move(blob));
    if (archive == nullptr)
    {
        return false;
    }

    RHI::IRHIDevice* device = nullptr;
    if (engine_ != nullptr)
    {
        if (auto* rhi = engine_->GetSubsystem<RHI::RHISubsystem>())
        {
            device = rhi->GetDevice();
        }
    }

    auto moduleCache = std::make_unique<ShaderModuleCache>(device, archive.get());
    auto globalShaderMap =
        std::make_unique<GlobalShaderMap>(archive.get(), moduleCache.get());
    auto materialMaps =
        std::make_unique<MaterialShaderMapCache>(archive.get(), moduleCache.get());

    archive_         = std::move(archive);
    moduleCache_     = std::move(moduleCache);
    globalShaderMap_ = std::move(globalShaderMap);
    materialMaps_    = std::move(materialMaps);

    lastObservedTimestamp_ = config_.blobStore->LastModified(config_.archiveKey);

    HYLUX_LOG_INFO(LogShader,
                   "ShaderSubsystem: opened archive '{}' via {} ({} pass, {} material)",
                   config_.archiveKey, config_.blobStore->DebugName(),
                   archive_->PassEntryCount(), archive_->MaterialEntryCount());
    return true;
}

void ShaderSubsystem::DropArchiveLocked() noexcept
{
    materialMaps_.reset();
    globalShaderMap_.reset();
    moduleCache_.reset();
    archive_.reset();
    lastObservedTimestamp_.reset();
}

void ShaderSubsystem::ReloadAll()
{
    DropArchiveLocked();
    if (!OpenArchiveLocked())
    {
        HYLUX_LOG_WARN(LogShader, "ShaderSubsystem::ReloadAll: archive open failed; staying empty");
        return;
    }

    HYLUX_LOG_INFO(LogShader, "ShaderSubsystem::ReloadAll: notifying {} subscriber(s)",
                   reloadCallbacks_.size());

    for (auto& [id, callback] : reloadCallbacks_)
    {
        if (callback)
        {
            callback();
        }
    }
}

void ShaderSubsystem::OnHotReloadPoll()
{
    const auto now = config_.blobStore->LastModified(config_.archiveKey);
    if (!now.has_value())
    {
        return;
    }
    if (!lastObservedTimestamp_.has_value() || *now != *lastObservedTimestamp_)
    {
        HYLUX_LOG_INFO(LogShader,
                       "ShaderSubsystem::HotReload: '{}' changed; reloading archive",
                       config_.archiveKey);
        ReloadAll();
    }
}

ShaderRef ShaderSubsystem::GetPassShader(std::uint64_t passNameHash,
                                         std::uint64_t permutationKey,
                                         RHI::ShaderStage stage)
{
    if (globalShaderMap_ == nullptr)
    {
        return ShaderRef{};
    }
    return globalShaderMap_->Get(passNameHash, permutationKey, stage);
}

MaterialShaderMap*
ShaderSubsystem::GetOrLoadMaterialShaderMap(std::uint64_t materialAssetHash)
{
    if (materialMaps_ == nullptr)
    {
        return nullptr;
    }
    return materialMaps_->GetOrLoad(materialAssetHash);
}

std::size_t ShaderSubsystem::SubscribeArchiveReloaded(ArchiveReloadedCallback callback)
{
    const std::size_t id = nextCallbackId_++;
    reloadCallbacks_.emplace(id, std::move(callback));
    return id;
}

void ShaderSubsystem::UnsubscribeArchiveReloaded(std::size_t id)
{
    reloadCallbacks_.erase(id);
}

} // namespace Hylux::Shader
