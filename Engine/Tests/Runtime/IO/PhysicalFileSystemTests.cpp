/// @file
/// @brief Tests for PhysicalFileSystem (StdFile / StdFileSystem on desktop).

#include "Core/IO/IFile.h"
#include "Core/IO/IFileSystem.h"
#include "Core/IO/PhysicalFileSystem.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("IO");

#include <array>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>

using namespace Hylux;

namespace
{

std::filesystem::path UniqueTempDir(const char* suffix)
{
    std::random_device rd;
    const auto stamp = std::to_string(rd()) + "_" + std::to_string(rd());
    auto path = std::filesystem::temp_directory_path() / ("hylux_pfs_" + std::string(suffix) + "_" + stamp);
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

} // namespace

TEST_CASE("PhysicalFileSystem::Create returns a usable IFileSystem on desktop")
{
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    CHECK_FALSE(fs->CurrentDirectory().empty());
}

TEST_CASE("StdFileSystem: Open Write creates a new file; Read of the file returns its bytes")
{
    TempDirGuard guard{"open"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);

    const auto path = (guard.path / "hello.txt").string();
    {
        auto file = fs->Open(path, FileOpenMode::Write);
        REQUIRE(file);
        const std::string payload{"Hi"};
        CHECK(file->Write(payload.data(), payload.size()) == payload.size());
        file->Flush();
    }
    REQUIRE(fs->Exists(path));

    auto file = fs->Open(path, FileOpenMode::Read);
    REQUIRE(file);
    CHECK(file->IsValid());
    char buf[3] = {};
    CHECK(file->Read(buf, 2) == 2u);
    CHECK(std::string(buf, 2) == "Hi");
    CHECK(file->Size() == 2u);
    CHECK(file->Read(buf, 4) == 0u);  // past EOF
}

TEST_CASE("StdFileSystem::Open: missing file with Read returns nullptr; bytes==0 short-circuits")
{
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    CHECK_FALSE(fs->Open("Z:/__no__/file.bin", FileOpenMode::Read));

    TempDirGuard guard{"empty_read"};
    const auto path = (guard.path / "x.bin").string();
    auto w = fs->Open(path, FileOpenMode::Write);
    REQUIRE(w);
    char b{};
    CHECK(w->Read(&b, 0) == 0u);
    CHECK(w->Write(&b, 0) == 0u);
}

TEST_CASE("StdFile: Seek + Tell across origins")
{
    TempDirGuard guard{"seek"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    const auto path = (guard.path / "seek.bin").string();
    auto w = fs->Open(path, FileOpenMode::Write);
    const char data[] = "ABCDEFGH";
    w->Write(data, 8);
    w->Flush();
    w.reset();

    auto r = fs->Open(path, FileOpenMode::Read);
    REQUIRE(r);
    CHECK(r->Tell() == 0);
    CHECK(r->Seek(2, SeekOrigin::Begin));
    CHECK(r->Tell() == 2);
    CHECK(r->Seek(1, SeekOrigin::Current));
    CHECK(r->Tell() == 3);
    CHECK(r->Seek(-1, SeekOrigin::End));
    CHECK(r->Tell() == 7);
}

TEST_CASE("StdFileSystem: Append positions at end and extends the file")
{
    TempDirGuard guard{"append"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    const auto path = (guard.path / "log.txt").string();
    {
        auto w = fs->Open(path, FileOpenMode::Write);
        w->Write("aa", 2);
    }
    {
        auto a = fs->Open(path, FileOpenMode::Append);
        REQUIRE(a);
        CHECK(a->Write("bb", 2) == 2u);
    }
    auto r = fs->Open(path, FileOpenMode::Read);
    char buf[5] = {};
    CHECK(r->Read(buf, 4) == 4u);
    CHECK(std::string(buf, 4) == "aabb");
}

TEST_CASE("StdFileSystem: ReadWrite creates an empty file when missing; preserves contents when present")
{
    TempDirGuard guard{"rw"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    const auto path = (guard.path / "rw.bin").string();

    auto rw = fs->Open(path, FileOpenMode::ReadWrite);
    REQUIRE(rw);
    CHECK(rw->Size() == 0u);
    rw->Write("hi", 2);
    rw.reset();

    auto rw2 = fs->Open(path, FileOpenMode::ReadWrite);
    REQUIRE(rw2);
    CHECK(rw2->Size() == 2u);
}

TEST_CASE("StdFileSystem: Exists / Stat / Remove / Rename")
{
    TempDirGuard guard{"stat"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);

    const auto missing = (guard.path / "no.bin").string();
    CHECK_FALSE(fs->Exists(missing));
    const FileStat missStat = fs->Stat(missing);
    CHECK_FALSE(missStat.exists);

    const auto present = (guard.path / "yes.bin").string();
    {
        auto w = fs->Open(present, FileOpenMode::Write);
        w->Write("hi", 2);
    }
    CHECK(fs->Exists(present));
    const FileStat st = fs->Stat(present);
    CHECK(st.exists);
    CHECK(st.isRegularFile);
    CHECK_FALSE(st.isDirectory);
    CHECK(st.size == 2u);

    const auto renamed = (guard.path / "yes2.bin").string();
    CHECK(fs->Rename(present, renamed));
    CHECK(fs->Exists(renamed));
    CHECK_FALSE(fs->Exists(present));

    CHECK(fs->Remove(renamed));
    CHECK_FALSE(fs->Exists(renamed));

    CHECK_FALSE(fs->Rename(missing, renamed));
}

TEST_CASE("StdFileSystem: CreateDirectories is idempotent; Stat on dir reports isDirectory")
{
    TempDirGuard guard{"dir"};
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    const auto nested = (guard.path / "a" / "b" / "c").string();
    CHECK(fs->CreateDirectories(nested));
    CHECK(fs->Exists(nested));
    const FileStat st = fs->Stat(nested);
    CHECK(st.exists);
    CHECK(st.isDirectory);
    CHECK_FALSE(st.isRegularFile);
    // Idempotent
    CHECK(fs->CreateDirectories(nested));
}

TEST_CASE("StdFileSystem: IsAbsolute / JoinPath / CurrentDirectory")
{
    auto fs = PhysicalFileSystem::Create();
    REQUIRE(fs != nullptr);
    CHECK_FALSE(fs->IsAbsolute("relative/path"));
#if defined(_WIN32)
    CHECK(fs->IsAbsolute("C:\\Windows"));
#else
    CHECK(fs->IsAbsolute("/usr"));
#endif
    const auto joined = fs->JoinPath("a", "b");
    CHECK(joined.find("a") != std::string::npos);
    CHECK(joined.find("b") != std::string::npos);
    CHECK_FALSE(fs->CurrentDirectory().empty());
}

TEST_SUITE_END();
