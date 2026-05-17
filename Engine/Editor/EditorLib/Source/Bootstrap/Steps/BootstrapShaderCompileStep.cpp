/// @file
/// @brief BootstrapShaderCompileStep implementation.

#include "Bootstrap/Steps/BootstrapShaderCompileStep.h"

#include "Bootstrap/BootstrapProgress.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "ShaderCompiler/ShaderCompiler.h"

#include <QString>

namespace Hylux::Editor
{

bool BootstrapShaderCompileStep::Run(BootstrapContext& context)
{
    if (context.repoRoot.empty())
    {
        HYLUX_LOG_WARN(LogRender, "BootstrapShaderCompileStep: repo root unknown; skipping");
        return true;
    }
    ShaderCompiler compiler;
    if (!compiler.IsReady())
    {
        HYLUX_LOG_ERROR(LogRender, "BootstrapShaderCompileStep: compiler not ready");
        return false;
    }

    ShaderCompiler::Config config;
    config.sourceDir     = context.repoRoot / "Engine" / "Shaders";
    config.outputArchive = context.executableDir / "ShaderCache" / "Win_Vulkan.hslib";

    auto progress = [&context](std::size_t index, std::size_t total, const std::string& relPath) {
        if (context.progress == nullptr || total == 0)
        {
            return;
        }
        const int percent = static_cast<int>((index * 100) / total);
        context.progress->SetPercent(percent);
        context.progress->SetMessage(QString::fromStdString(relPath));
    };

    const auto result = compiler.CompileIfOutdated(config, progress);
    if (!result.ok)
    {
        HYLUX_LOG_ERROR(LogRender, "BootstrapShaderCompileStep: compile failed");
        return false;
    }
    if (context.progress != nullptr)
    {
        const QString summary = result.compiled
                                    ? QString("Compiled %1 of %2 source files")
                                          .arg(result.outdatedCount)
                                          .arg(result.totalSources)
                                    : QString("Archive up to date (%1 sources)").arg(result.totalSources);
        context.progress->SetMessage(summary);
    }
    return true;
}

} // namespace Hylux::Editor
