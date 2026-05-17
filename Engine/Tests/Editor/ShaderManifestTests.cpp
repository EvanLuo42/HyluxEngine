/// @file
/// @brief Tests for the shader manifest sidecar: round-trip + mismatch detection.

#include "ShaderCompiler/ShaderManifest.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

using namespace Hylux::Editor;

namespace
{

[[nodiscard]] std::filesystem::path UniqueTempDir(std::string_view prefix)
{
    auto base = std::filesystem::temp_directory_path() / std::filesystem::path(prefix);
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);
    return base;
}

void WriteFile(const std::filesystem::path& p, std::string_view content)
{
    std::ofstream s(p, std::ios::binary);
    s.write(content.data(), static_cast<std::streamsize>(content.size()));
}

} // namespace

TEST_CASE("ShaderManifest round-trips entries through save/load")
{
    const auto tempDir = UniqueTempDir("hylux-manifest-roundtrip");
    WriteFile(tempDir / "A.slang", "// hello");
    WriteFile(tempDir / "B.slang", "// world");

    const ShaderManifest first    = ShaderManifest::BuildFromSourceDir(tempDir);
    const auto           filePath = tempDir / "manifest.slangindex";
    REQUIRE(first.Save(filePath));

    ShaderManifest loaded;
    REQUIRE(loaded.Load(filePath));
    CHECK(loaded.MatchesContent(first));
}

TEST_CASE("ShaderManifest detects content change")
{
    const auto tempDir = UniqueTempDir("hylux-manifest-detect-change");
    WriteFile(tempDir / "A.slang", "// v1");
    const ShaderManifest v1 = ShaderManifest::BuildFromSourceDir(tempDir);

    WriteFile(tempDir / "A.slang", "// v2 modified");
    const ShaderManifest v2 = ShaderManifest::BuildFromSourceDir(tempDir);

    CHECK_FALSE(v1.MatchesContent(v2));
}

TEST_CASE("ShaderManifest detects added file")
{
    const auto tempDir = UniqueTempDir("hylux-manifest-detect-added");
    WriteFile(tempDir / "A.slang", "// a");
    const ShaderManifest before = ShaderManifest::BuildFromSourceDir(tempDir);

    WriteFile(tempDir / "B.slang", "// b");
    const ShaderManifest after = ShaderManifest::BuildFromSourceDir(tempDir);

    CHECK_FALSE(before.MatchesContent(after));
}
