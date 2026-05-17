/// @file
/// @brief Tests for FilesystemBlobStore + IMappedBlob.

#include "Core/IO/Blob/FilesystemBlobStore.h"
#include "Core/IO/Blob/IMappedBlob.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("IO");

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

using namespace Hylux;

namespace
{

std::filesystem::path UniqueTempDir(const char* suffix)
{
    std::random_device rd;
    const auto stamp = std::to_string(rd()) + "_" + std::to_string(rd());
    auto path = std::filesystem::temp_directory_path() / ("hylux_blob_" + std::string(suffix) + "_" + stamp);
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

void Write(const std::filesystem::path& p, std::string_view content)
{
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
}

} // namespace

TEST_CASE("FilesystemBlobStore: missing key reports false and OpenMapped returns nullptr")
{
    TempDirGuard guard{"missing"};
    FilesystemBlobStore store{guard.path};
    CHECK(store.Root() == guard.path);
    CHECK_FALSE(store.Exists("nope.bin"));
    CHECK(store.OpenMapped("nope.bin") == nullptr);
    CHECK_FALSE(store.LastModified("nope.bin").has_value());
}

TEST_CASE("FilesystemBlobStore: Exists/LastModified/OpenMapped on a populated file")
{
    TempDirGuard guard{"hit"};
    Write(guard.path / "blob.bin", "hello-world");

    FilesystemBlobStore store{guard.path};
    CHECK(store.Exists("blob.bin"));
    CHECK(store.LastModified("blob.bin").has_value());

    auto blob = store.OpenMapped("blob.bin");
    REQUIRE(blob);
    auto bytes = blob->Bytes();
    REQUIRE(bytes.size() == 11u);
    CHECK(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()) == "hello-world");
    CHECK_FALSE(blob->DebugSource().empty());
}

TEST_CASE("FilesystemBlobStore: zero-byte file produces an empty span (HeapBlob fallback)")
{
    TempDirGuard guard{"empty"};
    Write(guard.path / "zero.bin", "");
    FilesystemBlobStore store{guard.path};
    auto blob = store.OpenMapped("zero.bin");
    REQUIRE(blob);
    CHECK(blob->Bytes().empty());
}

TEST_CASE("FilesystemBlobStore: a directory entry is not 'Exists' (Exists is for regular files)")
{
    TempDirGuard guard{"dir"};
    std::filesystem::create_directory(guard.path / "subdir");
    FilesystemBlobStore store{guard.path};
    CHECK_FALSE(store.Exists("subdir"));
}

TEST_CASE("FilesystemBlobStore::DebugName starts with 'filesystem:'")
{
    TempDirGuard guard{"name"};
    FilesystemBlobStore store{guard.path};
    const auto name = store.DebugName();
    CHECK(name.rfind("filesystem:", 0) == 0);
}

TEST_SUITE_END();
