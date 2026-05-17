/// @file
/// @brief Editor-side Slang → SPIR-V shader compiler. Walks a directory of .slang source
///        files, compiles every entry point to SPIR-V via the Slang C++ API, and packs
///        the results into an .hslib archive consumable by Hylux::Shader::ShaderSubsystem.
///        Stage 4b compiler: every entry uses permutationKey = 0; permutation
///        enumeration + material shader support land later.

#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>

namespace Hylux::Editor
{

/// @brief Compiler façade. Owns the Slang global session for the lifetime of the
///        editor process; Compile() is reentrant but not thread-safe.
class ShaderCompiler
{
public:
    struct Config
    {
        std::filesystem::path sourceDir;       // e.g. <repo>/Engine/Shaders
        std::filesystem::path outputArchive;   // e.g. <exe>/ShaderCache/Win_Vulkan.hslib
    };

    /// @brief Optional callback used by CompileIfOutdated to surface progress to the
    ///        editor splash. Called with the current file index (1-based), the total
    ///        file count, and the relative path being compiled.
    using ProgressFn = std::function<void(std::size_t index, std::size_t total, const std::string& relPath)>;

    /// @brief Outcome of CompileIfOutdated. `compiled` is true when the archive was
    ///        (re)written; `outdatedCount` is the number of source files that triggered
    ///        the rebuild (zero means the manifest matched and nothing was done).
    struct IncrementalResult
    {
        bool        ok            = false;
        bool        compiled      = false;
        std::size_t outdatedCount = 0;
        std::size_t totalSources  = 0;
    };

    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    /// @brief Compiles every .slang file under config.sourceDir and writes the archive.
    ///        Returns true when at least one shader compiled successfully and the archive
    ///        was persisted; logs warnings on per-file failures (those files are dropped
    ///        from the archive but do not abort the build).
    [[nodiscard]] bool Compile(const Config& config);

    /// @brief Loads the sidecar manifest written next to outputArchive (if any), walks
    ///        sourceDir and compares hashes, and only invokes the underlying compiler
    ///        when something is outdated or the archive is missing. On success writes
    ///        a fresh manifest for the next launch.
    [[nodiscard]] IncrementalResult CompileIfOutdated(const Config& config, ProgressFn progress = {});

    [[nodiscard]] bool IsReady() const noexcept;

private:
    struct Impl;
    Impl* impl_{nullptr};
};

} // namespace Hylux::Editor
