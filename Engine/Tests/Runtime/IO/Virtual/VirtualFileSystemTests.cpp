/// @file
/// @brief Tests for VirtualFileSystem mount management + IFileSystem façade.

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("IO::Virtual");

#include <filesystem>
#include <fstream>
#include <memory>
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
    auto path = std::filesystem::temp_directory_path() / ("hylux_vfs_" + std::string(suffix) + "_" + stamp);
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

void Touch(const std::filesystem::path& p, std::string_view content = "x")
{
    std::filesystem::create_directories(p.parent_path());
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::shared_ptr<IFileProvider> Loose(const std::filesystem::path& root)
{
    return std::make_shared<LooseFileProvider>(root);
}

} // namespace

TEST_CASE("VirtualFileSystem::Mount: invalid prefix or null provider returns Invalid")
{
    VirtualFileSystem vfs;
    CHECK(vfs.Mount("Engine/", Loose("dummy"), 0) == MountId::Invalid);
    CHECK(vfs.Mount("/Engine", Loose("dummy"), 0) == MountId::Invalid);
    CHECK(vfs.Mount("/Engine/", nullptr, 0) == MountId::Invalid);
}

TEST_CASE("VirtualFileSystem::Mount + EnumerateMounts: each mount gets a unique increasing id")
{
    TempDirGuard guard{"ids"};
    VirtualFileSystem vfs;
    const MountId a = vfs.Mount("/Engine/", Loose(guard.path), 0);
    const MountId b = vfs.Mount("/Game/",   Loose(guard.path), 0);
    REQUIRE(a != MountId::Invalid);
    REQUIRE(b != MountId::Invalid);
    CHECK(a != b);
    const auto mounts = vfs.EnumerateMounts();
    CHECK(mounts.size() == 2u);
}

TEST_CASE("VirtualFileSystem::Unmount: known id removes; unknown returns false")
{
    TempDirGuard guard{"unmount"};
    VirtualFileSystem vfs;
    const MountId id = vfs.Mount("/Engine/", Loose(guard.path), 0);
    REQUIRE(id != MountId::Invalid);
    CHECK_FALSE(vfs.Unmount(MountId::Invalid));
    CHECK(vfs.Unmount(id));
    CHECK_FALSE(vfs.Unmount(id));
    CHECK(vfs.EnumerateMounts().empty());
}

TEST_CASE("VirtualFileSystem::Open(Read) routes by longest-prefix winning")
{
    TempDirGuard a{"a"};
    TempDirGuard b{"b"};
    Touch(a.path / "x.bin", "engine");
    Touch(b.path / "x.bin", "engine-deep");

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(a.path), 0);
    vfs.Mount("/Engine/Deep/", Loose(b.path), 0);

    {
        auto f = vfs.Open("/Engine/x.bin", FileOpenMode::Read);
        REQUIRE(f);
        char buf[7] = {};
        f->Read(buf, 6);
        CHECK(std::string(buf, 6) == "engine");
    }
    {
        auto f = vfs.Open("/Engine/Deep/x.bin", FileOpenMode::Read);
        REQUIRE(f);
        char buf[12] = {};
        f->Read(buf, 11);
        CHECK(std::string(buf, 11) == "engine-deep");
    }
}

TEST_CASE("VirtualFileSystem: priority tiebreak among same-prefix mounts; read falls through on miss")
{
    TempDirGuard low{"low"};
    TempDirGuard high{"high"};
    Touch(low.path / "only_low.bin", "L");
    Touch(high.path / "only_high.bin", "H");

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(low.path), 0);
    vfs.Mount("/Engine/", Loose(high.path), 100);

    // High priority wins where both exist.
    Touch(low.path / "shared.bin", "L");
    Touch(high.path / "shared.bin", "H");
    auto fHi = vfs.Open("/Engine/shared.bin", FileOpenMode::Read);
    REQUIRE(fHi);
    char hb[1] = {}; fHi->Read(hb, 1);
    CHECK(hb[0] == 'H');

    // Fall through to low when high misses.
    auto fLo = vfs.Open("/Engine/only_low.bin", FileOpenMode::Read);
    REQUIRE(fLo);
    char lb[1] = {}; fLo->Read(lb, 1);
    CHECK(lb[0] == 'L');

    // Missing entirely.
    CHECK(vfs.Open("/Engine/nope.bin", FileOpenMode::Read) == nullptr);
}

TEST_CASE("VirtualFileSystem: Write only attempts top writer (no fall-through)")
{
    TempDirGuard a{"writer_a"};
    TempDirGuard b{"writer_b"};
    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(a.path), 0);
    vfs.Mount("/Engine/", Loose(b.path), 50);

    auto f = vfs.Open("/Engine/created.bin", FileOpenMode::Write);
    REQUIRE(f);
    f->Write("Z", 1);
    f.reset();
    CHECK(std::filesystem::exists(b.path / "created.bin"));
    CHECK_FALSE(std::filesystem::exists(a.path / "created.bin"));
}

TEST_CASE("VirtualFileSystem::Open(Read) on path-normalize failure returns nullptr")
{
    VirtualFileSystem vfs;
    CHECK(vfs.Open("relative.bin", FileOpenMode::Read) == nullptr);
    CHECK(vfs.Open("", FileOpenMode::Read) == nullptr);
    CHECK(vfs.Open("/Engine/../../x", FileOpenMode::Read) == nullptr);
}

TEST_CASE("VirtualFileSystem: Exists / Stat / Remove / CreateDirectories")
{
    TempDirGuard root{"stat"};
    Touch(root.path / "file.bin", "abc");
    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(root.path), 0);

    CHECK(vfs.Exists("/Engine/file.bin"));
    const auto st = vfs.Stat("/Engine/file.bin");
    CHECK(st.exists);
    CHECK(st.size == 3u);
    CHECK_FALSE(vfs.Exists("/Engine/nope.bin"));

    CHECK(vfs.CreateDirectories("/Engine/created"));
    CHECK(std::filesystem::exists(root.path / "created"));

    CHECK(vfs.Remove("/Engine/file.bin"));
    CHECK_FALSE(vfs.Exists("/Engine/file.bin"));
}

TEST_CASE("VirtualFileSystem::Rename always returns false (v1 unimplemented)")
{
    VirtualFileSystem vfs;
    CHECK_FALSE(vfs.Rename("/Engine/a.bin", "/Engine/b.bin"));
}

TEST_CASE("VirtualFileSystem: IsAbsolute / JoinPath / CurrentDirectory")
{
    VirtualFileSystem vfs;
    CHECK(vfs.IsAbsolute("/Engine/x"));
    CHECK(vfs.IsAbsolute("\\Engine\\x"));
    CHECK_FALSE(vfs.IsAbsolute("Engine/x"));
    CHECK_FALSE(vfs.IsAbsolute(""));

    CHECK(vfs.JoinPath("a", "b").find('a') != std::string::npos);
    CHECK(vfs.CurrentDirectory() == "/");
}

TEST_CASE("VirtualFileSystem::EnumerateFiles: walks the named mount; dedup by virtual path across same-prefix mounts")
{
    TempDirGuard low{"enum_low"};
    TempDirGuard high{"enum_high"};
    Touch(low.path / "shared.bin", "L");
    Touch(low.path / "only_low.bin", "L");
    Touch(high.path / "shared.bin", "H");
    Touch(high.path / "only_high.bin", "H");

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(low.path), 0);
    vfs.Mount("/Engine/", Loose(high.path), 100);

    std::set<std::string> seen;
    vfs.EnumerateFiles("/Engine/", true, [&](std::string_view path, const FileStat&) {
        seen.emplace(path);
    });
    CHECK(seen.count("/Engine/shared.bin") == 1);
    CHECK(seen.count("/Engine/only_low.bin") == 1);
    CHECK(seen.count("/Engine/only_high.bin") == 1);

    // Null visitor / invalid prefix should be silent no-ops.
    vfs.EnumerateFiles("/Engine/", true, {});
    vfs.EnumerateFiles("invalid", true, [](std::string_view, const FileStat&) {
        FAIL_CHECK("visitor invoked for invalid prefix");
    });
}

TEST_CASE("VirtualFileSystem: Shutdown clears mount table")
{
    TempDirGuard guard{"shutdown"};
    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", Loose(guard.path), 0);
    CHECK(vfs.EnumerateMounts().size() == 1u);
    vfs.Shutdown();
    CHECK(vfs.EnumerateMounts().empty());
}

TEST_SUITE_END();
