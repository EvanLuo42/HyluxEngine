/// @file
/// @brief Editor-side Slang → SPIR-V shader compiler. Walks a directory of .slang source
///        files, compiles every entry point to SPIR-V via the Slang C++ API, and packs
///        the results into an .hslib archive consumable by Hylux::Shader::ShaderSubsystem.
///        Stage 4b compiler: every entry uses permutationKey = 0; permutation
///        enumeration + material shader support land later.

#pragma once

#include <filesystem>

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

    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    /// @brief Compiles every .slang file under config.sourceDir and writes the archive.
    ///        Returns true when at least one shader compiled successfully and the archive
    ///        was persisted; logs warnings on per-file failures (those files are dropped
    ///        from the archive but do not abort the build).
    [[nodiscard]] bool Compile(const Config& config);

    [[nodiscard]] bool IsReady() const noexcept;

private:
    struct Impl;
    Impl* impl_{nullptr};
};

} // namespace Hylux::Editor
