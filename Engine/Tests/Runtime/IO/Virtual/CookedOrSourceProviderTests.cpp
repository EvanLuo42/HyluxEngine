/// @file
/// @brief Tests for CookedOrSourceProvider — cooked-first lookup, source fallback,
///        write-routing, null-source shipped-build behaviour.

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/CookedOrSourceProvider.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>

using namespace Hylux;

namespace
{

std::filesystem::path MakeTempDir(const char* tag)
{
    const auto base  = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto       dir   = base / (std::string{"hylux-cs-test-"} + tag + "-" + std::to_string(stamp));
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void RemoveAllSilent(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove_all(p, ec);
}

void WriteFile(const std::filesystem::path& path, std::string_view bytes)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::string ReadAll(IFile& f)
{
    std::string           out;
    std::array<char, 256> buf{};
    while (true)
    {
        const auto n = f.Read(buf.data(), buf.size());
        if (n == 0)
        {
            break;
        }
        out.append(buf.data(), static_cast<std::size_t>(n));
    }
    return out;
}

} // namespace

TEST_CASE("Read falls through to source when cooked is missing")
{
    const auto cookedDir = MakeTempDir("cooked-fallback");
    const auto sourceDir = MakeTempDir("source-fallback");
    WriteFile(sourceDir / "Materials" / "Foo.hmat", "source-payload");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    auto source = std::make_shared<LooseFileProvider>(sourceDir);
    CookedOrSourceProvider provider(cooked, source);

    auto file = provider.Open("Materials/Foo.hmat", FileOpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(ReadAll(*file) == "source-payload");
    CHECK(provider.Exists("Materials/Foo.hmat"));

    RemoveAllSilent(cookedDir);
    RemoveAllSilent(sourceDir);
}

TEST_CASE("Cooked shadows source when both exist (same logical name)")
{
    const auto cookedDir = MakeTempDir("cooked-shadow");
    const auto sourceDir = MakeTempDir("source-shadow");
    WriteFile(cookedDir / "Shared.txt", "cooked-wins");
    WriteFile(sourceDir / "Shared.txt", "source-loses");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    auto source = std::make_shared<LooseFileProvider>(sourceDir);
    CookedOrSourceProvider provider(cooked, source);

    auto file = provider.Open("Shared.txt", FileOpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(ReadAll(*file) == "cooked-wins");

    RemoveAllSilent(cookedDir);
    RemoveAllSilent(sourceDir);
}

TEST_CASE("Write goes only to the cooked root and does not fall through")
{
    const auto cookedDir = MakeTempDir("cooked-write");
    const auto sourceDir = MakeTempDir("source-write");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    auto source = std::make_shared<LooseFileProvider>(sourceDir);
    CookedOrSourceProvider provider(cooked, source);

    {
        auto writer = provider.Open("Output/Cube.hass", FileOpenMode::Write);
        REQUIRE(writer != nullptr);
        writer->Write("hass-bytes", 10);
    }

    CHECK(std::filesystem::exists(cookedDir / "Output" / "Cube.hass"));
    CHECK_FALSE(std::filesystem::exists(sourceDir / "Output" / "Cube.hass"));

    RemoveAllSilent(cookedDir);
    RemoveAllSilent(sourceDir);
}

TEST_CASE("Distinct extensions (cooked .hass and source .hmesh) coexist under one mount")
{
    const auto cookedDir = MakeTempDir("cooked-distinct");
    const auto sourceDir = MakeTempDir("source-distinct");
    WriteFile(cookedDir / "Meshes" / "Cube.hass", "cooked-cube");
    WriteFile(sourceDir / "Meshes" / "Cube.hmesh", R"({"type":"Mesh"})");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    auto source = std::make_shared<LooseFileProvider>(sourceDir);
    CookedOrSourceProvider provider(cooked, source);

    auto cookedFile = provider.Open("Meshes/Cube.hass", FileOpenMode::Read);
    REQUIRE(cookedFile != nullptr);
    CHECK(ReadAll(*cookedFile) == "cooked-cube");

    auto sourceFile = provider.Open("Meshes/Cube.hmesh", FileOpenMode::Read);
    REQUIRE(sourceFile != nullptr);
    CHECK(ReadAll(*sourceFile) == R"({"type":"Mesh"})");

    RemoveAllSilent(cookedDir);
    RemoveAllSilent(sourceDir);
}

TEST_CASE("Null source provider degrades to cooked-only (shipped build mode)")
{
    const auto cookedDir = MakeTempDir("cooked-only");
    WriteFile(cookedDir / "Foo.hass", "shipped");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    CookedOrSourceProvider provider(cooked, nullptr);

    auto file = provider.Open("Foo.hass", FileOpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(ReadAll(*file) == "shipped");
    CHECK_FALSE(provider.Open("missing.hass", FileOpenMode::Read));

    RemoveAllSilent(cookedDir);
}

TEST_CASE("Stat falls through to source when cooked is absent")
{
    const auto cookedDir = MakeTempDir("cooked-stat");
    const auto sourceDir = MakeTempDir("source-stat");
    WriteFile(sourceDir / "data.bin", "1234567890");

    auto cooked = std::make_shared<LooseFileProvider>(cookedDir);
    auto source = std::make_shared<LooseFileProvider>(sourceDir);
    CookedOrSourceProvider provider(cooked, source);

    const auto s = provider.Stat("data.bin");
    CHECK(s.exists);
    CHECK(s.size == 10);

    RemoveAllSilent(cookedDir);
    RemoveAllSilent(sourceDir);
}
