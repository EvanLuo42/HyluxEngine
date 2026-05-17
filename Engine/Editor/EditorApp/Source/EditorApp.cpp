/// @file
/// @brief EditorApp implementation.

#include "EditorApp.h"

#include "Core/IO/Blob/FilesystemBlobStore.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/LogSystem.h"
#include "Core/Logging/Logger.h"
#include "Core/Paths/EnginePaths.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIInstanceDesc.h"
#include "RHI/RHISubsystem.h"
#include "Shader/ShaderSubsystem.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>

namespace Hylux
{

EditorApp::EditorApp(int& argc, char** argv) : guiApp_(std::make_unique<QGuiApplication>(argc, argv))
{
    QGuiApplication::setApplicationName("HyluxEditor");
    QGuiApplication::setOrganizationName("Hylux");
    QQuickStyle::setStyle("Fusion");

    engine_.RegisterSubsystem<LogSystem>(LogSystemConfig{
        .async = false,
        .enableConsole = true,
        .enableFile = true,
        .enableDebugger = true,
        .logDirectory = (EnginePaths::ExecutableDir() / "Logs").string(),
    });
    auto* vfs = engine_.RegisterSubsystem<VirtualFileSystem>();

    RHI::InstanceDesc rhiInstanceDesc{};
    rhiInstanceDesc.applicationName = "HyluxEditor";
    RHI::DeviceDesc rhiDeviceDesc{};
#if !defined(NDEBUG)
    rhiInstanceDesc.gapiValidation = RHI::GapiValidationLevel::Standard;
    rhiDeviceDesc.gapiValidation   = RHI::GapiValidationLevel::Standard;
#endif
    engine_.RegisterSubsystem<RHI::RHISubsystem>(rhiInstanceDesc, rhiDeviceDesc);

    Shader::ShaderSubsystem::Config shaderConfig{};
    shaderConfig.blobStore =
        std::make_unique<FilesystemBlobStore>(EnginePaths::ExecutableDir() / "ShaderCache");
    shaderConfig.archiveKey      = "Win_Vulkan.hslib";
    shaderConfig.enableHotReload = true;
    engine_.RegisterSubsystem<Shader::ShaderSubsystem>(std::move(shaderConfig));

    engine_.Initialize();

    HYLUX_LOG_INFO(LogEngine, "HyluxEditor starting (Qt {}.{}.{})", QT_VERSION_MAJOR, QT_VERSION_MINOR,
                   QT_VERSION_PATCH);

    const auto& repoRoot       = EnginePaths::RepoRoot();
    const auto& contentRoot    = EnginePaths::ProjectContentRoot();
    if (repoRoot.empty())
    {
        HYLUX_LOG_WARN(LogEngine,
                       "VFS: could not discover repo root (no vcpkg.json found walking up from exe)");
    }
    else
    {
        vfs->Mount("/Engine/", std::make_shared<LooseFileProvider>(repoRoot / "Engine"), 0);
    }
    if (!contentRoot.empty())
    {
        vfs->Mount("/Game/", std::make_shared<LooseFileProvider>(contentRoot), 0);
    }

    QObject::connect(guiApp_.get(), &QCoreApplication::aboutToQuit, guiApp_.get(), [this]() {
        HYLUX_LOG_INFO(LogEngine, "HyluxEditor shutting down");
        if (tickTimer_)
        {
            tickTimer_->stop();
        }
        engine_.Shutdown();
    });

    lastTick_ = std::chrono::steady_clock::now();
    tickTimer_ = std::make_unique<QTimer>();
    QObject::connect(tickTimer_.get(), &QTimer::timeout, guiApp_.get(), [this]() {
        const auto now = std::chrono::steady_clock::now();
        const float delta = std::chrono::duration<float>(now - lastTick_).count();
        lastTick_ = now;
        engine_.Tick(delta);
    });
    tickTimer_->start(16);

    qmlEngine_ = std::make_unique<QQmlApplicationEngine>();
    QObject::connect(
        qmlEngine_.get(), &QQmlApplicationEngine::objectCreationFailed, guiApp_.get(),
        []() {
            HYLUX_LOG_FATAL(LogEngine, "QML root object failed to load");
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    qmlEngine_->loadFromModule("Hylux.Editor", "Main");
}

EditorApp::~EditorApp() = default;

int EditorApp::Run()
{
    return QGuiApplication::exec();
}

} // namespace Hylux
