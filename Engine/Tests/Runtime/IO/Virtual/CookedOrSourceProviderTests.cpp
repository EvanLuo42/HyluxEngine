/// @file
/// @brief Tests for CookedOrSourceProvider's two-tier fallback semantics.

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/CookedOrSourceProvider.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"

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
    auto path = std::filesystem::temp_directory_path() / ("hylux_cos_" + std::string(suffix) + "_" + stamp);
    std::filesystem::create_directories(path);
    return path;
}

struct TempRoots
{
    std::filesystem::path cookedDir;
    std::filesystem::path sourceDir;
    explicit TempRoots(const char* tag)
        : cookedDir(UniqueTempDir((std::string(tag) + "_cooked").c_str())),
          sourceDir(UniqueTempDir((std::string(tag) + "_source").c_str())) {}
    ~TempRoots()
    {
        std::error_code ec;
        std::filesystem::remove_all(cookedDir, ec);
        std::filesystem::remove_all(sourceDir, ec);
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

TEST_CASE("CookedOrSourceProvider: cooked-only when source is null still functions for cooked content")
{
    TempRoots roots{"only_cooked"};
    Touch(roots.cookedDir / "x.bin", "cooked");
    CookedOrSourceProvider cos{Loose(roots.cookedDir), nullptr};
    CHECK(cos.SupportsWrite());
    CHECK(cos.Exists("x.bin"));
    auto f = cos.Open("x.bin", FileOpenMode::Read);
    REQUIRE(f);
    char buf[6] = {};
    f->Read(buf, 6);
    CHECK(std::string(buf, 6) == "cooked");
}

TEST_CASE("CookedOrSourceProvider: cooked-null source-only — SupportsWrite false; read-only fallback")
{
    TempRoots roots{"only_source"};
    Touch(roots.sourceDir / "x.bin", "from-source");
    CookedOrSourceProvider cos{nullptr, Loose(roots.sourceDir)};
    CHECK_FALSE(cos.SupportsWrite());
    CHECK(cos.Exists("x.bin"));

    auto f = cos.Open("x.bin", FileOpenMode::Read);
    REQUIRE(f);
    char buf[11] = {};
    f->Read(buf, 11);
    CHECK(std::string(buf, 11) == "from-source");

    // Writes never go to source.
    CHECK(cos.Open("y.bin", FileOpenMode::Write) == nullptr);
    CHECK_FALSE(cos.CreateDirectories("z"));
    CHECK_FALSE(cos.Remove("x.bin"));
}

TEST_CASE("CookedOrSourceProvider: Read prefers cooked; falls through to source on miss")
{
    TempRoots roots{"fallthrough"};
    Touch(roots.cookedDir / "only_cooked.bin", "C");
    Touch(roots.sourceDir / "only_source.bin", "S");
    Touch(roots.cookedDir / "both.bin", "C");
    Touch(roots.sourceDir / "both.bin", "S");

    CookedOrSourceProvider cos{Loose(roots.cookedDir), Loose(roots.sourceDir)};

    auto a = cos.Open("only_cooked.bin", FileOpenMode::Read);
    REQUIRE(a);
    char ab[1] = {}; a->Read(ab, 1);
    CHECK(ab[0] == 'C');

    auto b = cos.Open("only_source.bin", FileOpenMode::Read);
    REQUIRE(b);
    char bb[1] = {}; b->Read(bb, 1);
    CHECK(bb[0] == 'S');

    auto c = cos.Open("both.bin", FileOpenMode::Read);
    REQUIRE(c);
    char cb[1] = {}; c->Read(cb, 1);
    CHECK(cb[0] == 'C');
}

TEST_CASE("CookedOrSourceProvider: Write/Append/ReadWrite never fall through")
{
    TempRoots roots{"no_fallthrough"};
    CookedOrSourceProvider cos{Loose(roots.cookedDir), Loose(roots.sourceDir)};
    // The file does not exist in either root. Write goes to cooked (cooked supports writes).
    auto f = cos.Open("new.bin", FileOpenMode::Write);
    REQUIRE(f);
    f->Write("z", 1);
    f.reset();
    CHECK(std::filesystem::exists(roots.cookedDir / "new.bin"));
    CHECK_FALSE(std::filesystem::exists(roots.sourceDir / "new.bin"));
}

TEST_CASE("CookedOrSourceProvider: Exists is OR of both providers; Stat prefers cooked")
{
    TempRoots roots{"exists"};
    Touch(roots.sourceDir / "src.bin", "source-only");
    Touch(roots.cookedDir / "ck.bin", "cooked");
    Touch(roots.sourceDir / "ck.bin", "source-shadowed");

    CookedOrSourceProvider cos{Loose(roots.cookedDir), Loose(roots.sourceDir)};
    CHECK(cos.Exists("src.bin"));
    CHECK(cos.Exists("ck.bin"));
    CHECK_FALSE(cos.Exists("missing.bin"));

    CHECK(cos.Stat("ck.bin").size == 6u);  // "cooked" wins
    CHECK(cos.Stat("src.bin").size == 11u);
    CHECK_FALSE(cos.Stat("missing.bin").exists);
}

TEST_CASE("CookedOrSourceProvider::EnumerateFiles: cooked first; source dedups by sub-path")
{
    TempRoots roots{"enum"};
    Touch(roots.cookedDir / "a.bin", "C");
    Touch(roots.cookedDir / "sub" / "b.bin", "C");
    Touch(roots.sourceDir / "a.bin", "S");          // shadowed
    Touch(roots.sourceDir / "sub" / "c.bin", "S");  // unique

    CookedOrSourceProvider cos{Loose(roots.cookedDir), Loose(roots.sourceDir)};
    std::set<std::string> seen;
    cos.EnumerateFiles("", true, [&](std::string_view p, const FileStat&) { seen.emplace(p); });

    CHECK(seen.count("a.bin") == 1);
    CHECK(seen.count("sub/b.bin") == 1);
    CHECK(seen.count("sub/c.bin") == 1);

    // Null visitor is a no-op.
    cos.EnumerateFiles("", true, {});
}

TEST_CASE("CookedOrSourceProvider::DebugName lists both child providers")
{
    TempRoots roots{"name"};
    CookedOrSourceProvider cos{Loose(roots.cookedDir), Loose(roots.sourceDir)};
    std::string name{cos.DebugName()};
    CHECK(name.find("CookedOrSource(") != std::string::npos);
    CHECK(name.find("cooked=") != std::string::npos);
    CHECK(name.find("source=") != std::string::npos);

    CookedOrSourceProvider missing{nullptr, nullptr};
    const std::string missingName{missing.DebugName()};
    CHECK(missingName.find("<null>") != std::string::npos);
}

TEST_CASE("CookedOrSourceProvider: Cooked() / Source() accessors return raw pointers")
{
    TempRoots roots{"accessors"};
    auto cookedShared = Loose(roots.cookedDir);
    CookedOrSourceProvider cos{cookedShared, nullptr};
    CHECK(cos.Cooked() == cookedShared.get());
    CHECK(cos.Source() == nullptr);
}

TEST_SUITE_END();
