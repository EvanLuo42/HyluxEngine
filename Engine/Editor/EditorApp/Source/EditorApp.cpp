/// @file
/// @brief EditorApp implementation: drives splash → bootstrap → EngineLoop →
///        DockHost/MainWindow sequence.

#include "EditorApp.h"

#include "Bootstrap/BootstrapProgress.h"
#include "Bootstrap/EditorBootstrap.h"
#include "Bootstrap/Steps/BootstrapShaderCompileStep.h"
#include "Context/EditorContext.h"
#include "Core/IO/Blob/FilesystemBlobStore.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/LogSystem.h"
#include "Core/Logging/Logger.h"
#include "Core/Paths/EnginePaths.h"
#include "Dock/BuiltinDocks.h"
#include "Dock/DockHost.h"
#include "Dock/DockRegistry.h"
#include "Engine/EngineLoop.h"
#include "Logging/QmlLogSink.h"
#include "MainWindow/MainWindowController.h"
#include "Menu/MenuRegistry.h"
#include "RHI/RHIDeviceDesc.h"
#include "RHI/RHIInstanceDesc.h"
#include "RHI/RHISubsystem.h"
#include "Renderer/Subsystem/RenderSubsystem.h"
#include "Selection/SelectionContext.h"
#include "Shader/ShaderSubsystem.h"
#include "Splash/SplashController.h"
#include "Threading/MainThreadDispatcher.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QTimer>

namespace Hylux
{

EditorApp::EditorApp(int& argc, char** argv) : guiApp_(std::make_unique<QGuiApplication>(argc, argv))
{
    QGuiApplication::setApplicationName("HyluxEditor");
    QGuiApplication::setOrganizationName("Hylux");
    QQuickStyle::setStyle("Fusion");

    splash_ = std::make_unique<SplashController>();
    splash_->Show();

    QObject::connect(guiApp_.get(), &QCoreApplication::aboutToQuit, guiApp_.get(),
                     [this]() { ShutdownLoopAndEngine(); });

    QTimer::singleShot(0, guiApp_.get(), [this]() { ContinueBootstrap(); });
}

EditorApp::~EditorApp() = default;

int EditorApp::Run()
{
    return QGuiApplication::exec();
}

void EditorApp::RegisterEditorSubsystems()
{
    auto qmlSink   = std::make_unique<Editor::QmlLogSink>();
    logSinkPtr_    = qmlSink.get();

    LogSystemConfig logConfig{};
    logConfig.async          = false;
    logConfig.enableConsole  = true;
    logConfig.enableFile     = true;
    logConfig.enableDebugger = true;
    logConfig.logDirectory   = (EnginePaths::ExecutableDir() / "Logs").string();
    logConfig.extraSinks.push_back(std::move(qmlSink));
    engine_.RegisterSubsystem<LogSystem>(std::move(logConfig));

    engine_.RegisterSubsystem<VirtualFileSystem>();

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

    Renderer::RendererConfig rendererConfig{};
    rendererConfig.psoCacheDir = EnginePaths::ExecutableDir() / "PsoCache";
    engine_.RegisterSubsystem<Renderer::RenderSubsystem>(std::move(rendererConfig));
}

void EditorApp::RegisterBuiltinMenus()
{
    using Editor::MenuActionDesc;
    menuRegistry_->Register(MenuActionDesc{
        .path = "File/Exit", .shortcut = "Ctrl+Q", .run = []() { QCoreApplication::quit(); }, .order = 0});
    menuRegistry_->Register(MenuActionDesc{.path = "Edit/Clear Selection",
                                           .shortcut = "Esc",
                                           .run      = [this]() { selectionContext_->Clear(); },
                                           .order    = 0});
    menuRegistry_->Register(MenuActionDesc{
        .path = "Tools/Reload Shaders",
        .shortcut = "F5",
        .run      = [this]() {
            editorContext_->Submit([](Engine& engine) {
                if (auto* shaders = engine.GetSubsystem<Shader::ShaderSubsystem>(); shaders != nullptr)
                {
                    shaders->ReloadAll();
                }
            });
        },
        .order = 0});
    menuRegistry_->Register(MenuActionDesc{.path = "Help/About", .run = []() {}, .order = 0});
}

void EditorApp::ContinueBootstrap()
{
    RegisterEditorSubsystems();

    Editor::EditorBootstrap bootstrap;
    bootstrap.AddStep(std::make_unique<Editor::BootstrapShaderCompileStep>());

    Editor::BootstrapContext ctx{};
    ctx.engine        = &engine_;
    ctx.progress      = splash_->Progress();
    ctx.repoRoot      = EnginePaths::RepoRoot();
    ctx.executableDir = EnginePaths::ExecutableDir();

    if (!bootstrap.Run(ctx))
    {
        HYLUX_LOG_FATAL(LogEngine, "Editor bootstrap failed; exiting");
        QCoreApplication::exit(-1);
        return;
    }

    splash_->SetStatus("Initializing subsystems…");

    engineLoop_ = std::make_unique<EngineLoop>(engine_);
    Future<bool> initFut = engineLoop_->Start();
    dispatcher_          = std::make_unique<Editor::MainThreadDispatcher>();
    dispatcher_->Then<bool>(std::move(initFut),
                            [this](bool& ok) { OnEngineInitialized(ok); });
}

void EditorApp::OnEngineInitialized(bool ok)
{
    if (!ok)
    {
        HYLUX_LOG_FATAL(LogEngine, "Engine initialization failed; exiting");
        QCoreApplication::exit(-1);
        return;
    }

    const auto repoRoot    = EnginePaths::RepoRoot();
    const auto contentRoot = EnginePaths::ProjectContentRoot();
    engineLoop_->SubmitCommand([repoRoot, contentRoot](Engine& engine) {
        if (auto* vfs = engine.GetSubsystem<VirtualFileSystem>(); vfs != nullptr)
        {
            if (!repoRoot.empty())
            {
                vfs->Mount("/Engine/", std::make_shared<LooseFileProvider>(repoRoot / "Engine"), 0);
            }
            if (!contentRoot.empty())
            {
                vfs->Mount("/Game/", std::make_shared<LooseFileProvider>(contentRoot), 0);
            }
        }
    });

    selectionContext_ = std::make_unique<Editor::SelectionContext>();
    menuRegistry_     = std::make_unique<Editor::MenuRegistry>();
    dockRegistry_     = std::make_unique<Editor::DockRegistry>();
    dockHost_         = std::make_unique<Editor::DockHost>();

    Editor::RegisterBuiltinDocks(*dockRegistry_, logSinkPtr_);

    editorContext_ = std::make_unique<Editor::EditorContext>(*engineLoop_, *dispatcher_, *dockRegistry_,
                                                             *dockHost_, *menuRegistry_, *selectionContext_);

    dockHost_->Build(*editorContext_, *dockRegistry_);
    RegisterBuiltinMenus();

    mainWindow_ = std::make_unique<MainWindowController>(*editorContext_);
    mainWindow_->Load();

    splash_->Close();
    HYLUX_LOG_INFO(LogEngine, "HyluxEditor ready");
}

void EditorApp::ShutdownLoopAndEngine()
{
    HYLUX_LOG_INFO(LogEngine, "HyluxEditor shutting down");
    if (mainWindow_)
    {
        mainWindow_->TearDown();
        mainWindow_.reset();
    }
    if (dockHost_)
    {
        dockHost_->TearDown();
    }
    if (engineLoop_)
    {
        engineLoop_->Stop();
        engineLoop_.reset();
    }
    editorContext_.reset();
    dockHost_.reset();
    dockRegistry_.reset();
    menuRegistry_.reset();
    selectionContext_.reset();
    dispatcher_.reset();
}

} // namespace Hylux
