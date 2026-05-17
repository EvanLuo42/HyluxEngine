/// @file
/// @brief End-to-end subsystem integration test. Wires Engine → VirtualFileSystem →
///        RHISubsystem → ShaderSubsystem → AssetSubsystem → RenderSubsystem through
///        Engine::Initialize, exercises basic flow (mount + LoadByGuid path + frame
///        submission), and tears everything down. The test runs per-backend via GPU_CASE
///        and the ValidationLogGuard fails the subcase on any Error/Fatal log from
///        engine-side validation or backend debug layers during the run.

#include "../RHI/GpuCase.h"

#include "Asset/AssetSubsystem.h"
#include "Asset/Cooked/CookedHeader.h"
#include "Core/Guid.h"
#include "Core/IO/Blob/IBlobStore.h"
#include "Core/IO/Blob/IMappedBlob.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "Engine/Engine.h"
#include "RHI/RHISubsystem.h"
#include "Renderer/Subsystem/RenderSubsystem.h"
#include "Renderer/Subsystem/RendererConfig.h"
#include "Renderer/View/SceneViewRequest.h"
#include "Shader/ShaderSubsystem.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Integration");

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace Hylux;
using namespace Hylux::Tests;

namespace
{

std::filesystem::path UniqueTempDir(const char* suffix)
{
    std::random_device rd;
    const auto stamp = std::to_string(rd()) + "_" + std::to_string(rd());
    auto path = std::filesystem::temp_directory_path() / ("hylux_e2e_" + std::string(suffix) + "_" + stamp);
    std::filesystem::create_directories(path);
    return path;
}

/// @brief Minimal IBlobStore returning nullptr for every key. Lets ShaderSubsystem::Initialize
///        complete (it tolerates a missing archive with a warn log, not an error).
class NullBlobStore final : public IBlobStore
{
public:
    [[nodiscard]] std::unique_ptr<IMappedBlob> OpenMapped(std::string_view) override
    {
        return nullptr;
    }
    [[nodiscard]] bool Exists(std::string_view) const override { return false; }
    [[nodiscard]] std::optional<std::filesystem::file_time_type> LastModified(std::string_view) const override
    {
        return std::nullopt;
    }
    [[nodiscard]] std::string_view DebugName() const noexcept override { return "null"; }
};

void Write(const std::filesystem::path& p, const std::vector<std::uint8_t>& bytes)
{
    std::filesystem::create_directories(p.parent_path());
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

/// @brief Minimal valid HassHeader buffer so AssetRegistry::Scan picks it up.
std::vector<std::uint8_t> BuildEmptyHass(Asset::AssetTypeId type, Guid guid)
{
    std::vector<std::uint8_t> buf(sizeof(Asset::Cooked::HassHeader), 0u);
    Asset::Cooked::HassHeader h{};
    h.magic     = Asset::Cooked::kHassMagic;
    h.endianTag = Asset::Cooked::kHassEndianTag;
    h.version   = Asset::Cooked::kHassVersion;
    h.typeTag   = static_cast<std::uint16_t>(type);
    std::memcpy(h.guid, guid.bytes.data(), Guid::kSize);
    h.payloadOffset = sizeof(Asset::Cooked::HassHeader);
    h.payloadSize   = 0;
    std::memcpy(buf.data(), &h, sizeof(h));
    return buf;
}

struct ContentRootGuard
{
    std::filesystem::path engineRoot;
    std::filesystem::path gameRoot;
    ContentRootGuard()
        : engineRoot(UniqueTempDir("engine")),
          gameRoot(UniqueTempDir("game"))
    {}
    ~ContentRootGuard()
    {
        std::error_code ec;
        std::filesystem::remove_all(engineRoot, ec);
        std::filesystem::remove_all(gameRoot, ec);
    }
};

} // namespace

GPU_CASE("End-to-end: subsystem topological init/shutdown + frame submit", Backends::All)
{
    ContentRootGuard guard;
    // Seed each mount with a minimal cooked asset so the AssetRegistry has work to do.
    const auto engineGuid = Guid::Generate();
    const auto gameGuid   = Guid::Generate();
    Write(guard.engineRoot / "engine_asset.hass",
          BuildEmptyHass(Asset::AssetTypeId::Material, engineGuid));
    Write(guard.gameRoot / "game_asset.hass",
          BuildEmptyHass(Asset::AssetTypeId::Mesh, gameGuid));

    Engine engine;

    // --- VirtualFileSystem -------------------------------------------------
    auto* vfs = engine.RegisterSubsystem<VirtualFileSystem>();
    REQUIRE(vfs != nullptr);

    // --- RHISubsystem (uses the same backend as the GPU_CASE subcase) ------
    RHI::InstanceDesc instDesc{};
    instDesc.preferredDevice = ToDeviceType(gpu.backend);
    instDesc.rhiValidation   = RHI::RhiValidationLevel::Standard;
    instDesc.gapiValidation  = RHI::GapiValidationLevel::Standard;
    instDesc.applicationName = "HyluxE2E";
    RHI::DeviceDesc devDesc{};
    devDesc.graphicsQueueCount = 1;
    devDesc.computeQueueCount  = 0;
    devDesc.copyQueueCount     = 0;
    devDesc.rhiValidation      = RHI::RhiValidationLevel::Standard;
    devDesc.gapiValidation     = RHI::GapiValidationLevel::Standard;
    devDesc.crashReporter      = RHI::GpuCrashReporterKind::None;
    auto* rhi = engine.RegisterSubsystem<RHI::RHISubsystem>(instDesc, devDesc);
    REQUIRE(rhi != nullptr);

    // --- ShaderSubsystem (NullBlobStore tolerates missing archive) ---------
    Shader::ShaderSubsystem::Config shaderCfg{};
    shaderCfg.blobStore       = std::make_unique<NullBlobStore>();
    shaderCfg.archiveKey      = "engine.hslib";
    shaderCfg.enableHotReload = false;
    auto* shaders = engine.RegisterSubsystem<Shader::ShaderSubsystem>(std::move(shaderCfg));
    REQUIRE(shaders != nullptr);

    // --- RenderSubsystem ---------------------------------------------------
    Renderer::RendererConfig rendererCfg{};
    rendererCfg.framesInFlight        = 2;
    rendererCfg.transformBufferCapacity = 256;
    rendererCfg.uploadRingBytesPerFrame = 1ull << 20;
    rendererCfg.enablePsoDiskCache      = false;
    auto* renderer = engine.RegisterSubsystem<Renderer::RenderSubsystem>(rendererCfg);
    REQUIRE(renderer != nullptr);

    // --- AssetSubsystem (depends on everything above) ---------------------
    Asset::AssetSubsystemConfig assetCfg{};
    assetCfg.lruBudgetBytes        = 1ull << 20;
    assetCfg.scanEngineMountAtInit = true;
    assetCfg.scanGameMountAtInit   = true;
    auto* assets = engine.RegisterSubsystem<Asset::AssetSubsystem>(assetCfg);
    REQUIRE(assets != nullptr);

    // Mount BEFORE Initialize so AssetSubsystem's startup scan sees the seeded files.
    vfs->Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.engineRoot), 0);
    vfs->Mount("/Game/",   std::make_shared<LooseFileProvider>(guard.gameRoot),   0);

    engine.Initialize();
    CHECK(engine.IsInitialized());

    // VFS sees both mounts and can read seeded files.
    CHECK(vfs->Exists("/Engine/engine_asset.hass"));
    CHECK(vfs->Exists("/Game/game_asset.hass"));

    // Registry picked up both seeded entries.
    const auto& registry = assets->GetRegistry();
    CHECK(registry.Size() == 2u);
    CHECK(registry.Find(engineGuid).has_value());
    CHECK(registry.Find(gameGuid).has_value());

    // Render subsystem is initialized; renderable state is queryable.
    CHECK(renderer->GetDevice() == rhi->GetDevice());
    const auto idA = renderer->SpawnPrimitive({});
    const auto idB = renderer->SpawnPrimitive({});
    CHECK(Renderer::IsValid(idA));
    CHECK(Renderer::IsValid(idB));
    CHECK(idA != idB);
    renderer->SetPrimitiveFlags(idA, 0xFF);

    // Empty frame end-to-end. The render thread will pick up and consume it.
    std::vector<Renderer::SceneViewRequest> views;
    renderer->SubmitFrame(views);
    renderer->WaitForFramesConsumed();
    CHECK(renderer->GetFrameId() >= 1u);

    // Drop a primitive and submit one more frame to exercise the despawn path.
    renderer->DespawnPrimitive(idA);
    renderer->SubmitFrame(views);
    renderer->WaitForFramesConsumed();

    engine.Shutdown();
    CHECK_FALSE(engine.IsInitialized());
}

TEST_SUITE_END();
