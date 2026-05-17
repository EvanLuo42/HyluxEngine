/// @file
/// @brief ShaderCompiler implementation. The Slang C++ API headers are wrapped behind a
///        PIMPL so consumers don't pull `<slang.h>` into TU's that just want to invoke
///        the editor compiler.

#include "ShaderCompiler/ShaderCompiler.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Utils/Hash.h"
#include "RHI/RHIEnums.h"
#include "ShaderCompiler/HslibBuilder.h"
#include "ShaderCompiler/ShaderManifest.h"

#include <slang-com-ptr.h>
#include <slang.h>

#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace Hylux::Editor
{
namespace
{

/// @brief Maps Slang's stage enum onto the engine's RHI::ShaderStage. Returns ::None
///        for stages the renderer does not yet route (raygen, etc.).
[[nodiscard]] RHI::ShaderStage MapSlangStage(SlangStage stage) noexcept
{
    switch (stage)
    {
        case SLANG_STAGE_VERTEX:        return RHI::ShaderStage::Vertex;
        case SLANG_STAGE_HULL:          return RHI::ShaderStage::Hull;
        case SLANG_STAGE_DOMAIN:        return RHI::ShaderStage::Domain;
        case SLANG_STAGE_GEOMETRY:      return RHI::ShaderStage::Geometry;
        case SLANG_STAGE_FRAGMENT:      return RHI::ShaderStage::Pixel;
        case SLANG_STAGE_COMPUTE:       return RHI::ShaderStage::Compute;
        case SLANG_STAGE_AMPLIFICATION: return RHI::ShaderStage::Task;
        case SLANG_STAGE_MESH:          return RHI::ShaderStage::Mesh;
        default:                        return RHI::ShaderStage::None;
    }
}

/// @brief Slang diagnostic blob → log line, when non-empty.
void LogDiagnostics(const char* prefix, slang::IBlob* diagnostics)
{
    if (diagnostics == nullptr || diagnostics->getBufferSize() == 0)
    {
        return;
    }
    const std::string_view text(static_cast<const char*>(diagnostics->getBufferPointer()),
                                 diagnostics->getBufferSize());
    HYLUX_LOG_WARN(LogRender, "{}: {}", prefix, text);
}

/// @brief Derives the engine-stable pass name from a .slang source path. The pass name
///        is the filename without extension; e.g. `Forward.Opaque.slang` → `Forward.Opaque`.
[[nodiscard]] std::string PassNameFromPath(const std::filesystem::path& slangPath)
{
    return slangPath.stem().string();
}

} // namespace

struct ShaderCompiler::Impl
{
    Slang::ComPtr<slang::IGlobalSession> globalSession;
};

ShaderCompiler::ShaderCompiler() : impl_(new Impl)
{
    if (SLANG_FAILED(slang::createGlobalSession(impl_->globalSession.writeRef())))
    {
        HYLUX_LOG_ERROR(LogRender, "ShaderCompiler: slang::createGlobalSession failed");
        impl_->globalSession.setNull();
    }
}

ShaderCompiler::~ShaderCompiler()
{
    delete impl_;
}

bool ShaderCompiler::IsReady() const noexcept
{
    return impl_ != nullptr && impl_->globalSession != nullptr;
}

bool ShaderCompiler::Compile(const Config& config)
{
    if (!IsReady())
    {
        HYLUX_LOG_ERROR(LogRender, "ShaderCompiler::Compile: compiler not initialized");
        return false;
    }
    if (!std::filesystem::is_directory(config.sourceDir))
    {
        HYLUX_LOG_WARN(LogRender, "ShaderCompiler::Compile: source dir '{}' does not exist",
                       config.sourceDir.string());
        return false;
    }

    // Per-compile session targets SPIR-V (Vulkan 1.3) and adds the source dir as the
    // module search path so .slang files can `import` each other freely.
    slang::TargetDesc targetDesc{};
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = impl_->globalSession->findProfile("spirv_1_5");
    targetDesc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

    const std::string sourceDirString = config.sourceDir.string();
    const char* searchPaths[] = { sourceDirString.c_str() };

    slang::SessionDesc sessionDesc{};
    sessionDesc.targets         = &targetDesc;
    sessionDesc.targetCount     = 1;
    sessionDesc.searchPaths     = searchPaths;
    sessionDesc.searchPathCount = 1;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(impl_->globalSession->createSession(sessionDesc, session.writeRef())))
    {
        HYLUX_LOG_ERROR(LogRender, "ShaderCompiler::Compile: createSession failed");
        return false;
    }

    HslibBuilder builder;
    std::size_t totalSourceFiles      = 0;
    std::size_t totalCompiledEntries  = 0;

    for (const auto& dirEntry : std::filesystem::directory_iterator(config.sourceDir))
    {
        if (!dirEntry.is_regular_file())
        {
            continue;
        }
        if (dirEntry.path().extension() != ".slang")
        {
            continue;
        }
        ++totalSourceFiles;

        const std::string passName     = PassNameFromPath(dirEntry.path());
        const std::uint64_t passHash   = Hash::Fnv1a64(passName);
        const std::uint64_t layoutHash = Hash::Fnv1a64(passName + ".Layout");
        const std::string  moduleName  = dirEntry.path().stem().string();

        Slang::ComPtr<slang::IBlob> diagnostics;
        slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
        LogDiagnostics("ShaderCompiler::loadModule diagnostics", diagnostics.get());
        if (module == nullptr)
        {
            HYLUX_LOG_WARN(LogRender, "ShaderCompiler: loadModule '{}' failed; skipping",
                           moduleName);
            continue;
        }

        const SlangInt32 entryPointCount = module->getDefinedEntryPointCount();
        if (entryPointCount == 0)
        {
            HYLUX_LOG_WARN(LogRender, "ShaderCompiler: module '{}' declares no entry points",
                           moduleName);
            continue;
        }

        // Stage 4b: push constant size is fixed to match RendererDrawPush (4 uint32s).
        // Every entry point in a module shares the same logical pipeline layout, so we
        // give them the same pipelineLayoutHash — PsoCache requires VS / PS to agree.
        constexpr std::uint32_t kRendererDrawPushSize = 16;

        for (SlangInt32 epIndex = 0; epIndex < entryPointCount; ++epIndex)
        {
            Slang::ComPtr<slang::IEntryPoint> entryPoint;
            if (SLANG_FAILED(module->getDefinedEntryPoint(epIndex, entryPoint.writeRef())))
            {
                continue;
            }

            const char* entryName = entryPoint->getFunctionReflection()->getName();

            slang::IComponentType* components[] = {
                static_cast<slang::IComponentType*>(module),
                static_cast<slang::IComponentType*>(entryPoint.get()),
            };
            Slang::ComPtr<slang::IComponentType> composed;
            if (SLANG_FAILED(session->createCompositeComponentType(
                    components, 2, composed.writeRef(), diagnostics.writeRef())))
            {
                LogDiagnostics("ShaderCompiler::createCompositeComponentType", diagnostics.get());
                continue;
            }

            Slang::ComPtr<slang::IComponentType> linked;
            if (SLANG_FAILED(composed->link(linked.writeRef(), diagnostics.writeRef())))
            {
                LogDiagnostics("ShaderCompiler::link", diagnostics.get());
                continue;
            }

            // Stage discovery via the linked program's reflection. Slang doesn't expose
            // stage directly on FunctionReflection — it lives on the EntryPointReflection
            // returned by the program layout.
            slang::ProgramLayout* layout = linked->getLayout(0, diagnostics.writeRef());
            if (layout == nullptr)
            {
                LogDiagnostics("ShaderCompiler::getLayout", diagnostics.get());
                continue;
            }
            slang::EntryPointReflection* epReflection = layout->findEntryPointByName(entryName);
            if (epReflection == nullptr)
            {
                HYLUX_LOG_WARN(LogRender,
                               "ShaderCompiler: '{}' entry '{}' not found in linked layout",
                               moduleName, entryName);
                continue;
            }
            const SlangStage       slangStage = epReflection->getStage();
            const RHI::ShaderStage rhiStage   = MapSlangStage(slangStage);
            if (rhiStage == RHI::ShaderStage::None)
            {
                HYLUX_LOG_WARN(LogRender, "ShaderCompiler: '{}' entry '{}' unsupported stage {}",
                               moduleName, entryName, static_cast<int>(slangStage));
                continue;
            }

            Slang::ComPtr<slang::IBlob> code;
            if (SLANG_FAILED(linked->getEntryPointCode(0, 0, code.writeRef(),
                                                       diagnostics.writeRef())) ||
                code == nullptr || code->getBufferSize() == 0)
            {
                LogDiagnostics("ShaderCompiler::getEntryPointCode", diagnostics.get());
                HYLUX_LOG_WARN(LogRender, "ShaderCompiler: '{}' entry '{}' produced no SPIR-V",
                               moduleName, entryName);
                continue;
            }

            HslibPassEntryInput entry{};
            entry.passNameHash           = passHash;
            entry.permutationKey         = 0;
            entry.stage                  = rhiStage;
            // Slang with GENERATE_SPIRV_DIRECTLY emits every SPIR-V entry point as "main"
            // regardless of the source function name. Match that so the pipeline shader
            // stage's pName resolves at vkCreateGraphicsPipelines.
            entry.entryPoint             = "main";
            entry.pushConstantSize       = kRendererDrawPushSize;
            entry.pushConstantVisibility = static_cast<std::uint32_t>(RHI::ShaderStage::All);
            entry.pipelineLayoutHash     = layoutHash;
            entry.sourceFilePath         = dirEntry.path().string();
            entry.sourceHash             = Hash::Fnv1a64(entry.sourceFilePath);

            const auto* bytes = static_cast<const std::byte*>(code->getBufferPointer());
            entry.spirv.assign(bytes, bytes + code->getBufferSize());

            HYLUX_LOG_INFO(LogRender,
                           "ShaderCompiler: '{}' entry '{}' stage={} → {} bytes SPIR-V",
                           moduleName, entry.entryPoint,
                           static_cast<std::uint32_t>(rhiStage), entry.spirv.size());

            builder.AddPassEntry(std::move(entry));
            ++totalCompiledEntries;
        }
    }

    if (totalCompiledEntries == 0)
    {
        HYLUX_LOG_WARN(LogRender,
                       "ShaderCompiler: no entries compiled (sourceFiles={}, sourceDir={})",
                       totalSourceFiles, config.sourceDir.string());
        return false;
    }

    if (!builder.WriteToFile(config.outputArchive))
    {
        HYLUX_LOG_ERROR(LogRender, "ShaderCompiler: failed to write archive '{}'",
                        config.outputArchive.string());
        return false;
    }

    HYLUX_LOG_INFO(LogRender,
                   "ShaderCompiler: archive '{}' written ({} entries from {} source files)",
                   config.outputArchive.string(), totalCompiledEntries, totalSourceFiles);
    return true;
}

namespace
{

[[nodiscard]] std::filesystem::path ManifestPathFor(const std::filesystem::path& archive)
{
    std::filesystem::path manifest = archive;
    manifest += ".slangindex";
    return manifest;
}

[[nodiscard]] std::size_t CountOutdated(const ShaderManifest& previous, const ShaderManifest& current)
{
    std::unordered_map<std::string, std::uint64_t> previousByPath;
    previousByPath.reserve(previous.Entries().size());
    for (const auto& entry : previous.Entries())
    {
        previousByPath.emplace(entry.relativePath, entry.contentHash);
    }
    std::size_t outdated = 0;
    for (const auto& entry : current.Entries())
    {
        auto it = previousByPath.find(entry.relativePath);
        if (it == previousByPath.end() || it->second != entry.contentHash)
        {
            ++outdated;
        }
    }
    for (const auto& entry : previous.Entries())
    {
        bool stillPresent = false;
        for (const auto& currentEntry : current.Entries())
        {
            if (currentEntry.relativePath == entry.relativePath)
            {
                stillPresent = true;
                break;
            }
        }
        if (!stillPresent)
        {
            ++outdated;
        }
    }
    return outdated;
}

} // namespace

ShaderCompiler::IncrementalResult ShaderCompiler::CompileIfOutdated(const Config& config, ProgressFn progress)
{
    IncrementalResult result{};

    const ShaderManifest current = ShaderManifest::BuildFromSourceDir(config.sourceDir);
    result.totalSources          = current.Entries().size();

    const std::filesystem::path manifestPath = ManifestPathFor(config.outputArchive);
    ShaderManifest              previous;
    const bool                  havePrevious = previous.Load(manifestPath);

    std::error_code ec;
    const bool      archiveExists = std::filesystem::exists(config.outputArchive, ec);

    if (havePrevious && archiveExists && previous.MatchesContent(current))
    {
        result.ok       = true;
        result.compiled = false;
        HYLUX_LOG_INFO(LogRender,
                       "ShaderCompiler: archive '{}' up to date ({} sources)",
                       config.outputArchive.string(), result.totalSources);
        return result;
    }

    if (result.totalSources == 0)
    {
        result.ok            = true;
        result.compiled      = false;
        result.outdatedCount = 0;
        HYLUX_LOG_INFO(LogRender,
                       "ShaderCompiler: source dir '{}' contains no .slang files; archive skipped",
                       config.sourceDir.string());
        return result;
    }

    result.outdatedCount = havePrevious ? CountOutdated(previous, current) : current.Entries().size();
    if (!archiveExists)
    {
        result.outdatedCount = std::max<std::size_t>(result.outdatedCount, current.Entries().size());
    }

    if (progress)
    {
        std::size_t index = 0;
        for (const auto& entry : current.Entries())
        {
            ++index;
            progress(index, current.Entries().size(), entry.relativePath);
        }
    }

    const bool compileOk = Compile(config);
    result.ok            = compileOk;
    result.compiled      = compileOk;
    if (compileOk)
    {
        if (!current.Save(manifestPath))
        {
            HYLUX_LOG_WARN(LogRender,
                           "ShaderCompiler: archive built but manifest '{}' could not be saved",
                           manifestPath.string());
        }
    }
    return result;
}

} // namespace Hylux::Editor
