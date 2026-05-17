/// @file
/// @brief Tests for LooseFileProvider (read/write/enumerate against a real disk directory).

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("IO::Virtual");

#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <string>

using namespace Hylux;

namespace
{

std::filesystem::path UniqueTempDir(const char* suffix)
{
    std::random_device rd;
    const auto stamp = std::to_string(rd()) + "_" + std::to_string(rd());
    auto path = std::filesystem::temp_directory_path() / ("hylux_loose_" + std::string(suffix) + "_" + stamp);
    std::filesystem::create_directories(path);
    return path;
}

struct TempDirGuard
{
    std::filesystem::path path;
    explicit TempDirGuard(const char* tag) : path(UniqueTempDir(tag)) {}
    ~TempDirGuard()
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void Touch(const std::filesystem::path& p, std::string_view content = "")
{
    std::filesystem::create_directories(p.parent_path());
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
}

} // namespace

TEST_CASE("LooseFileProvider: claims write support and produces a Loose(<root>) debug name")
{
    TempDirGuard guard{"dbg"};
    LooseFileProvider lp{guard.path};
    CHECK(lp.SupportsWrite());
    std::string dbg{lp.DebugName()};
    CHECK(dbg.rfind("Loose(", 0) == 0);
    CHECK(lp.RootDir() == guard.path);
}

TEST_CASE("LooseFileProvider::Open(Write) auto-creates parent directories")
{
    TempDirGuard guard{"autoparent"};
    LooseFileProvider lp{guard.path};
    auto file = lp.Open("a/b/c/file.bin", FileOpenMode::Write);
    REQUIRE(file);
    const char data[] = "hi";
    CHECK(file->Write(data, 2) == 2u);
    file.reset();
    CHECK(std::filesystem::exists(guard.path / "a" / "b" / "c" / "file.bin"));
}

TEST_CASE("LooseFileProvider::Open(Read) returns nullptr for missing path; Read after Write returns bytes")
{
    TempDirGuard guard{"read"};
    LooseFileProvider lp{guard.path};
    CHECK(lp.Open("missing.bin", FileOpenMode::Read) == nullptr);

    Touch(guard.path / "hi.bin", "ABCD");
    auto file = lp.Open("hi.bin", FileOpenMode::Read);
    REQUIRE(file);
    char buf[5] = {};
    CHECK(file->Read(buf, 4) == 4u);
    CHECK(std::string(buf, 4) == "ABCD");
}

TEST_CASE("LooseFileProvider::Exists / Stat: regular file vs missing")
{
    TempDirGuard guard{"stat"};
    LooseFileProvider lp{guard.path};
    Touch(guard.path / "file.bin", "1234");
    CHECK(lp.Exists("file.bin"));
    const FileStat st = lp.Stat("file.bin");
    CHECK(st.exists); CHECK(st.isRegularFile); CHECK(st.size == 4u);

    CHECK_FALSE(lp.Exists("nope.bin"));
    CHECK_FALSE(lp.Stat("nope.bin").exists);
}

TEST_CASE("LooseFileProvider::Remove / CreateDirectories")
{
    TempDirGuard guard{"rm"};
    LooseFileProvider lp{guard.path};
    CHECK(lp.CreateDirectories("nested/inside"));
    CHECK(std::filesystem::exists(guard.path / "nested" / "inside"));

    Touch(guard.path / "tmp.bin", "hi");
    CHECK(lp.Remove("tmp.bin"));
    CHECK_FALSE(lp.Exists("tmp.bin"));
    CHECK_FALSE(lp.Remove("tmp.bin"));
}

TEST_CASE("LooseFileProvider::EnumerateFiles: recursive vs non-recursive; empty subRoot scans the whole root")
{
    TempDirGuard guard{"enum"};
    Touch(guard.path / "a.bin");
    Touch(guard.path / "sub" / "b.bin");
    Touch(guard.path / "sub" / "deeper" / "c.bin");

    LooseFileProvider lp{guard.path};

    std::set<std::string> nonRecursive;
    lp.EnumerateFiles("", false, [&](std::string_view p, const FileStat&) {
        nonRecursive.emplace(p);
    });
    CHECK(nonRecursive.count("a.bin") == 1);
    // Non-recursive: 'sub/b.bin' should not appear.
    CHECK(nonRecursive.count("sub/b.bin") == 0);

    std::set<std::string> recursive;
    lp.EnumerateFiles("", true, [&](std::string_view p, const FileStat& st) {
        CHECK(st.isRegularFile);
        recursive.emplace(p);
    });
    CHECK(recursive.count("a.bin") == 1);
    CHECK(recursive.count("sub/b.bin") == 1);
    CHECK(recursive.count("sub/deeper/c.bin") == 1);
}

TEST_CASE("LooseFileProvider::EnumerateFiles: null visitor and missing subRoot are no-ops")
{
    TempDirGuard guard{"null_visitor"};
    LooseFileProvider lp{guard.path};
    // Null visitor: should not crash.
    lp.EnumerateFiles("", true, {});
    // Missing scanRoot: bail out silently.
    lp.EnumerateFiles("does_not_exist", true, [](std::string_view, const FileStat&) {
        FAIL_CHECK("visitor invoked for missing scanRoot");
    });
}

TEST_SUITE_END();
